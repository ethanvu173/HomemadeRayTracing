# Homemade Ray Tracing
A project to experiment with raytracing and explore multithreaded performance on both the CPU and GPU using CUDA.
## Requirements
* C++20 or higher
* CMake 3.18+
* [CUDA Toolkit 13.2+](https://developer.nvidia.com/cuda/toolkit)
* SFML 3+
## Build
On Windows:
1. Ensure all requirements above are installed.
2. Navigate to the directory *HomemadeRayTracing*. Create a directory named *build* and navigate to *build* in the terminal.
3. When building for the first time, run the following commands: <br>
```
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/Users/cielr/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```
4. For subsequent builds, only run:<br>
```cmake --build . --config Release```
## Usage
**When using for the first time**, navigate to the directory *HomemadeRayTracing* and create a directory named *output*.<br>
___
### Generating an image
Navigate to the directory *build* in the terminal. After building, enter the following command:<br>
```Release\raytracer.exe```<br>
The image will appear in a window. You will also find the image in the *output* directory. <br>
___
### Selecting rendering mode
This program has 3 rendering modes, a simple single-threaded process, a CPU multithreaded process, and a CUDA process. <br>
To change modes, edit the `render_mode` variable at the top of main.cpp and rebuild the program. <br>
___
### Adding objects
All the objects exist in the struct ```world```. There are 2 types of objects, **spheres** and **rectangles**. There are 3 types of materials, **lambertian** (which equally scatter light), **metal** (which is reflective), and **dielectric** (which is transparent).<br>
<br>
Objects can have a **colour**. A colour is initialized like so: ```colour(r, g, b)``` where ```r, g, b``` are doubles between 0-1.<br>
<br>
To add an object, first initialize its material. The initialization depends on material type:
* **Creating a Lambertian material:** <br>
```lambertian your_lambertian{colour(r, g, b)};```<br>
e.g. ```lambertian mat_diffuse{colour(0.8, 0.2, 0.2)};```
* **Metal:** <br>
```metal your_metal{colour(r, g, b), fuzz};  // double fuzz: 0 = perfect mirror, 1 = rough surface```<br>
e.g. ```metal mat_mirror{colour(0.9, 0.9, 0.9), 0.0};```
* **Dielectric:** <br>
```dielectric your_dielectric{refractive_index}  // refractive_index is a double```<br>
e.g. ```dielectric mat_glass{1.5};``` <br>
<br>
Next, add the object itself. After initializing the world as w, follow these steps: <br>

* **Adding spheres:** <br>
```w.spheres.push_back({ {x, y, z}, radius, &material});```<br>
e.g. ```w.spheres.push_back({{0, 0, -1.2}, 0.5, &mat_diffuse});```
* **Triangles:** <br>
```
w.triangles.push_back({vec3(x, y, z),  // Bottom left corner
                           vec3(x, y, z), // Bottom right corner
                           vec3(x, y, z), // Top centre corner
                           &material});
```
e.g. 
```
w.triangles.push_back({vec3(-2.5, -0.5, -2.5),
                           vec3(-0.5, -0.5, -2.5),
                           vec3(-1.5, 1.0, -2.0),
                           &mat_diffuse});
```
### Adding objects to the CUDA render
If you are rendering using CUDA, all materials must be added to the array `mat_list`. Below the initialization of your materials, add the addresses of your materials like so, in the same order they were initialized:
```
std::vector<material*> mat_list = {&mat_ground, &mat_diffuse, &mat_blue, 
                                        &mat_mirror, &mat_brushed, &mat_glass,
                                        &mat_diamond};
```
## Samples
*main.cpp* comes with a preset scene demonstrating the capabilities of the ray tracing engine. It includes multiple spheres and triangles with different materials and colours:<br>
<img width="600" height="338" alt="image" src="https://github.com/user-attachments/assets/7ec25d41-e5c7-4252-95ec-f5197bcebc9d" />
<br><br>
Below is a scene that replicates [Image 18](https://raytracing.github.io/images/img-1.18-glass-hollow.png) in Peter Shirley's book Ray Tracing in One Weekend:
<img width="600" height="338" alt="image" src="https://github.com/user-attachments/assets/65d0b6d3-3ecf-40b5-9194-ada3dff75c73" />
<br>
In *main.cpp*, replace the code underneath the comment ```// SCENE CONFIGURATION``` with the following:
```
    // SCENE CONFIGURATION
    // Material types
    lambertian mat_ground{colour(0.8, 0.8, 0)};
    lambertian mat_centre{colour(0.1, 0.2, 0.5)};
    dielectric mat_left{1.5};
    dielectric mat_bubble{1.0 / 1.5};
    metal mat_right{colour(0.8, 0.6, 0.2), 1.0};

    // Placing objects into the world
    world w;
    w.spheres.push_back({{0, -100.5, -1}, 100, &mat_ground});
    w.spheres.push_back({{0, 0, -1.2}, 0.5, &mat_centre});
    w.spheres.push_back({{0, 0, -1}, 0.5, &mat_left});
    w.spheres.push_back({{-1, 0, -1}, 0.4, &mat_bubble});
    w.spheres.push_back({{1, 0, -1}, 0.5, &mat_right});
```
## Results
While this program is a relatively simplistic ray tracing engine, it supports light reflection and refraction, essential for photorealistic scenes. This program also has several nice-to-haves, such as anti-alliasing and triangle shapes (which Ray Tracing in one Weekend does not have).<br>

As a performance demonstrator, this program clearly shows the advantages of CPU multithreading and CUDA over conventional single-threaded processes. When rendering the built-in scene on my system, I have observed roughly the following performance:
* CPU Single-thread: 12000 ms
* CPU Multithread: 1400 ms
* CUDA: 64 ms <br>

The extreme performance advantages of CUDA demonstrates its capabilities for smooth real-time rendering, should it be implemented.
## Roadmap
* Implement interactive camera movement
## Attributions
This project is based on Peter Shirley's book [Ray Tracing in One Weekend](https://raytracing.github.io/books/RayTracingInOneWeekend.html), with several of my own modifications.


