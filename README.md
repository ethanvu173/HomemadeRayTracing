# Homemade Ray Tracing
A project to experiment with raytracing and (eventually) explore multi-threaded performance on both the CPU and GPU using CUDA.
## Requirements
* CMake 3.18+
## Build
On Windows:
1. Navigate to the directory *HomemadeRayTracing*. Create a directory named *build* and navigate to *build* in the terminal.
2. When building for the first time, run the following commands: <br>
```
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/Users/cielr/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```
4. For subsequent builds, only run:<br>
```cmake --build . --config Release```
## Usage
**When using for the first time**, navigate to the directory *HomemadeRayTracing* and create a directory named *output*.<br>
### Generating an image
Navigate to the directory *build* in the terminal. After building, enter the following command:<br>
```Release\raytracer.exe ../output/image.ppm```<br>
You will find the image in the *output* directory. Put it into any ppm viewer to view it. 
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
## Samples
*main.cpp* comes with a preset scene demonstrating the capabilities of the ray tracing engine. It includes multiple spheres and triangles with different materials and colours:<br>
<img width="600" height="338" alt="image" src="https://github.com/user-attachments/assets/7ec25d41-e5c7-4252-95ec-f5197bcebc9d" />
<br><br>
Below is a scene that replicates Image 18 in Peter Shirley's book Ray Tracing in One Weekend:
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
## Roadmap
* Implement CPU and GPU multi-threading, using CUDA
## Attributions
This project is based on Peter Shirley's book [Ray Tracing in One Weekend](https://raytracing.github.io/books/RayTracingInOneWeekend.html), with several of my own modifications.


