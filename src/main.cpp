#include "camera.h"
#include "sphere.h"
#include "hittable.h"
#include "cpu_renderer.h"
#include "scene_gpu.h"
#include "gpu_renderer.cuh"
#include <chrono>
#include <iostream>
#include <SFML/Graphics.hpp>


enum mode {
    SIMPLE,
    MULTITHREAD,
    CUDA,
    REALTIME
};

mode render_mode = SIMPLE;

// Write the scene to the SFML image frame using the frame buffer pointer fb.
void SFML_write(sf::Image* frame, colour* fb, int H, int W) {
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const colour& c = fb[y*W+x];
            sf::Vector2u coords = {(unsigned) x, (unsigned) y};
            // Apply gamma correction to the colour
            uint8_t c_x = (uint8_t) (255.f * std::sqrt(std::clamp((float)c.x, 0.f, 1.f)));
            uint8_t c_y = (uint8_t) (255.f * std::sqrt(std::clamp((float)c.y, 0.f, 1.f)));
            uint8_t c_z = (uint8_t) (255.f * std::sqrt(std::clamp((float)c.z, 0.f, 1.f)));
            frame->setPixel(coords, sf::Color(c_x, c_y, c_z));
        }
    }
}

// Generate an SFML window and display the scene using the frame buffer fb.
// Save the image to the output directory as a png.
void SFML_render(std::vector<colour> fb, int H, int W) {
    sf::Image frame;
    frame.resize({(unsigned) W, (unsigned) H}, sf::Color::Black);
    sf::Vector2u dimensions = {(unsigned) W, (unsigned) H};
    sf::RenderWindow window(sf::VideoMode(dimensions), "Ray Tracer");
    sf::Texture tex;

    SFML_write(&frame, fb.data(), H, W);
    tex.loadFromImage(frame);
    sf::Sprite sprite(tex);

    frame.saveToFile("../output/image.png");

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }
        window.clear();
        window.draw(sprite);
        window.display();
    }
}

// CUDA version using CUDA supported uchar3
void SFML_write_cuda(sf::Image* frame, uchar3* fb, int H, int W) {
    for (int j = 0; j < H; ++j) {
        for (int i = 0; i < W; ++i) {
            const uchar3& c = fb[j*W+i];
            sf::Vector2u coords = {(unsigned) i, (unsigned) j};
            // Gamma correction already applied, do not apply now.
            uint8_t c_x = (uint8_t) c.x;
            uint8_t c_y = (uint8_t) c.y;
            uint8_t c_z = (uint8_t) c.z;
            frame->setPixel(coords, sf::Color(c_x, c_y, c_z));
        }
    }
}

void SFML_render_cuda(std::vector<uchar3> fb, int H, int W) {
    sf::Image frame;
    sf::Vector2u dimensions = {(unsigned) W, (unsigned) H};
    frame.resize(dimensions, sf::Color::Black);
    sf::RenderWindow window(sf::VideoMode(dimensions), "Ray Tracer");
    sf::Texture tex;

    SFML_write_cuda(&frame, fb.data(), H, W);
    tex.loadFromImage(frame);
    sf::Sprite sprite(tex);

    frame.saveToFile("../output/image.png");

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }
        window.clear();
        window.draw(sprite);
        window.display();
    }
}

void simple_render(camera cam, int image_width, int image_height, int samples_per_pixel,
                    world w, int max_depth) {
    // Generate the image
    std::vector<colour> frame_buffer(image_height*image_width);

    auto t1 = std::chrono::high_resolution_clock::now();

    for (int j = 0; j < image_height; j++) {
        std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; i++) {   
            colour pixel_colour{0, 0, 0};

            // Random sampling for anti-aliasing
            for (int sample = 0; sample < samples_per_pixel; sample++) {
                double r_u = rand_double() - 0.5;
                double r_v = rand_double() - 0.5;
                vec3 centre = cam.pixel00_loc + (i+r_u)*cam.pixel_delta_u + (j+r_v)*cam.pixel_delta_v;
                ray r{cam.origin, (centre-cam.origin).unit_vector()};
                pixel_colour += ray_colour(r, w, max_depth);
            }
            // Get the average pixel colour from all samples taken
            pixel_colour = pixel_colour / samples_per_pixel;
            frame_buffer[j*image_width+i] = pixel_colour;
        }
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
    std::clog << "\rRendered in " << (int) ms << " ms\n";

    SFML_render(frame_buffer, image_height, image_width);
}

int main() {
    const int samples_per_pixel = 100;
    const int max_depth = 50;
    double aspect_ratio = 16.0 /9.0;
    int image_width = 800;
    int image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    int block_size = 16;  // GPU block size for CUDA rendering

    camera cam = camera(image_width, image_height);

    // SCENE CONFIGURATION
    // Material types
    lambertian mat_ground{colour(0.45, 0.45, 0.45)};
    lambertian mat_diffuse{colour(0.8, 0.2, 0.2)};
    lambertian mat_blue{colour(0.2, 0.3, 0.9)};
    metal mat_mirror{colour(0.9, 0.9, 0.9), 0.0};
    metal mat_brushed{colour(0.8, 0.5, 0.1), 0.4};
    dielectric mat_glass{1.5};
    dielectric mat_diamond{2.4};

    lambertian mat_tri1{colour(0.2, 0.8, 0.3)};
    lambertian mat_tri2{colour(0.9, 0.7, 0.1)};

    // Array of materials for CUDA rendering
    std::vector<material*> mat_list = {&mat_ground, &mat_diffuse, &mat_blue, 
                                        &mat_mirror, &mat_brushed, &mat_glass,
                                        &mat_diamond, &mat_tri1, &mat_tri2};

    // Placing objects into the world
    world w;
    w.spheres.push_back({{0, -100.5, -1}, 100 ,&mat_ground});
    w.spheres.push_back({{0, 0, -1.2}, 0.5, &mat_diffuse});
    w.spheres.push_back({{-1.1, 0, -1}, 0.5, &mat_glass});
    w.spheres.push_back({{-1.1, 0, -1}, -0.45, &mat_glass}); // hollow interior for glass sphere
    w.spheres.push_back({{1.1, 0, -1}, 0.5, &mat_brushed});
    w.spheres.push_back({{0.0, -0.3, -0.6}, 0.2, &mat_mirror});
    w.spheres.push_back({{0.0, 0.25, -0.5}, 0.1, &mat_diamond});
    w.spheres.push_back({{0.0, 0.1, 0.5}, 0.4, &mat_blue});
    w.triangles.push_back({vec3(-2.5, -0.5, -2.5),
                           vec3(-0.5, -0.5, -2.5),
                           vec3(-1.5, 1.0, -2.0),
                           &mat_tri1});
    w.triangles.push_back({vec3(0.75, -0.5, -3.0),
                           vec3(1.75, -0.5, -3.0),
                           vec3(1.25, 1.2, -3.0),
                           &mat_tri2});
    
    if (render_mode == SIMPLE)
        simple_render(cam, image_width, image_height, samples_per_pixel, w, max_depth);

    else if (render_mode == MULTITHREAD) {
        // Number of threads (by default, the max threads on the device)
        int threads = std::thread::hardware_concurrency();
        std::vector<colour> frame_buffer(image_height*image_width);

        auto t1 = std::chrono::high_resolution_clock::now();

        render_sections(frame_buffer.data(), image_width, image_height, 
                        cam, w, samples_per_pixel, max_depth,
                        64, threads);

        auto t2 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        std::clog << "Rendered in " << (int) ms << " ms\n";
        
        SFML_render(frame_buffer, image_height, image_width);
    }

    else if (render_mode == CUDA) {
        scene_gpu scene = upload_scene(w, mat_list, cam);
        uchar3* d_frame_buffer = nullptr;

        auto t1 = std::chrono::high_resolution_clock::now();
        cuda_render(&d_frame_buffer, image_width, image_height, scene,
                    samples_per_pixel, max_depth, block_size);
        auto t2 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        std::clog << "Rendered in " << (int) ms << " ms\n";
        
        // Copy the CUDA-generated frame buffer to the main CPU process.
        std::vector<uchar3> h_frame_buffer(image_width * image_height);
        cuda_check(cudaMemcpy(h_frame_buffer.data(), d_frame_buffer, 
                    image_width * image_height * sizeof(uchar3),
                    cudaMemcpyDeviceToHost));
        
        cuda_check(cudaFree(d_frame_buffer));
        free_scene_gpu(scene);
        
        SFML_render_cuda(h_frame_buffer, image_height, image_width);
    }

    else if (render_mode == REALTIME) {
        // Keep a version of the camera's lookfrom and lookat vectors to update
        // within the loop below
        vec3 lookfrom = {0, 0, 0};
        vec3 lookat = {0, 0, -1};
        double step = 0.03;  // Camera movement step
        int frames = 0;
        sf::Clock clock; // for FPS counter
        
        // Render each frame with the GPU
        scene_gpu scene = upload_scene(w, mat_list, cam);
        uchar3* d_frame_buffer = nullptr;

        // SFML window initialization
        sf::Image frame;
        sf::Vector2u dimensions = {(unsigned) image_width, (unsigned) image_height};
        frame.resize(dimensions, sf::Color::Black);
        sf::RenderWindow window(sf::VideoMode(dimensions), "Ray Tracer");
        sf::Texture tex;

        // Render the first frame, then display it when the window opens
        cuda_render(&d_frame_buffer, image_width, image_height, scene,
                    samples_per_pixel, max_depth, block_size);
        std::vector<uchar3> h_frame_buffer(image_width * image_height);
        cuda_check(cudaMemcpy(h_frame_buffer.data(), d_frame_buffer, 
                    image_width * image_height * sizeof(uchar3),
                    cudaMemcpyDeviceToHost));
        
        SFML_write_cuda(&frame, h_frame_buffer.data(), image_height, image_width);
        tex.loadFromImage(frame);
        sf::Sprite sprite(tex);

        while (window.isOpen()) {
            while (auto event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) window.close();
                if (auto* key_pressed = event->getIf<sf::Event::KeyPressed>()) {
                    switch (key_pressed->code) {
                        case sf::Keyboard::Key::Escape:
                            window.close();
                            break;
                        case sf::Keyboard::Key::A:
                            lookfrom -= cam.u * step;
                            lookat -= cam.u * step;
                            break;
                        case sf::Keyboard::Key::D:
                            lookfrom += cam.u * step;
                            lookat += cam.u * step;
                            break;
                        case sf::Keyboard::Key::W:
                            lookfrom -= cam.w * step;
                            lookat -= cam.w * step;
                            break;
                        case sf::Keyboard::Key::S:
                            lookfrom += cam.w * step;
                            lookat += cam.w * step;
                            break;
                        case sf::Keyboard::Key::Q:
                            lookfrom += cam.v * step;
                            lookat += cam.v * step;
                            break;
                        case sf::Keyboard::Key::E:
                            lookfrom -= cam.v * step;
                            lookat -= cam.v * step;
                            break;
                        case sf::Keyboard::Key::Right:
                            lookat.x = lookat.x * std::cos(-step) + lookat.z * std::sin(-step);
                            lookat.z = -lookat.x * std::sin(-step) + lookat.z * std::cos(-step);
                            break;
                        case sf::Keyboard::Key::Left:
                            lookat.x = lookat.x * std::cos(step) + lookat.z * std::sin(step);
                            lookat.z = -lookat.x * std::sin(step) + lookat.z * std::cos(step);
                            break;
                        // Naive up and down implementation. More sophisticated implementation coming soon.
                        case sf::Keyboard::Key::Up:
                            lookat += {0, step, 0};
                            break;
                        case sf::Keyboard::Key::Down:
                            lookat -= {0, step, 0};
                            break;
                    }
                }
            }

            // Update the camera after changing its position and orientation
            cam = camera(image_width, image_height, lookfrom, lookat);
            scene = upload_scene(w, mat_list, cam);

            // Render a new frame
            cuda_render(&d_frame_buffer, image_width, image_height, scene,
                        40, 25, block_size);

            std::vector<uchar3> h_frame_buffer(image_width * image_height);
            cuda_check(cudaMemcpy(h_frame_buffer.data(), d_frame_buffer, 
                                    image_width * image_height * sizeof(uchar3),
                                    cudaMemcpyDeviceToHost));
            
            SFML_write_cuda(&frame, h_frame_buffer.data(), image_height, image_width);
            tex.loadFromImage(frame);
            sf::Sprite sprite(tex);

            window.clear();
            window.draw(sprite);
            window.display();

            cuda_check(cudaFree(d_frame_buffer));
            d_frame_buffer = nullptr;
            free_scene_gpu(scene);

            // FPS counter
            frames++;
            if (clock.getElapsedTime().asSeconds() >= 1.0) {
                window.setTitle("Ray Tracer. FPS: " + std::to_string(frames));
                frames = 0;
                clock.restart();
            }
        }
    }
}