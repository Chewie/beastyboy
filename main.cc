#define GLM_FORCE_RADIANS
#include <vector>
#include <iostream>
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const size_t num_particles = 20000;

const GLchar* vertex_shader = R"SHADER(
#version 430

in vec4 position;
in vec4 color;

uniform mat4 view;
uniform mat4 projection;

out vec4 Color;

void main()
{
    Color = color;
    gl_Position = projection * view * vec4(position.xyz, 1.0);
}
)SHADER";

const GLchar* geometry_shader = R"SHADER(
#version 430

layout(points) in;
layout(triangle_strip, max_vertices = 6) out;

in vec4 Color[];
out vec4 fcolor;

void main()
{
    fcolor = Color[0];
    gl_Position = gl_in[0].gl_Position + vec4(-0.01, 0, 0, 0);
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + vec4(0.01, 0, 0, 0);
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + vec4(0, -0.01, 0, 0);
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + vec4(0, 0.01, 0, 0);
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + vec4(0, 0.01, 0, 0);
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + vec4(0, -0.01, 0, 0);
    EmitVertex();
    EndPrimitive();
}
)SHADER";

const GLchar* fragment_shader = R"SHADER(
#version 430

in vec4 fcolor;
out vec4 outColor;

void main()
{
    outColor = vec4(fcolor.xyz, 1.0);
}
)SHADER";


const GLchar* compute_shader = R"SHADER(
#version 430
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable

struct particle
{
    vec4 pos;
    vec4 color;
};

layout(std430, binding=0) buffer particles
{
    particle p[];
};

layout(std430, binding=1) buffer velocities
{
    vec4 v[];
};

uniform float elapsed_time;
uniform bool flip;

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;
void main()
{
    uint N = gl_NumWorkGroups.x * gl_WorkGroupSize.x;
    uint idx = gl_GlobalInvocationID.x;
    vec3 acc = vec3(0, 0, 0);

    // Double buffering hack
    if (flip)
    {
        for (int i = 0; i < N; i += 2)
            {
                vec3 diff = p[idx].pos.xyz - p[i].pos.xyz;
                float distSqr = dot(diff, diff) + 0.001;
                float distSixth = distSqr * distSqr * distSqr;
                float invDistCube = 1.0f/sqrt(distSixth);
                acc -= diff * 40 * invDistCube;
                if (length(acc) > 2.0)
                    acc = normalize(acc) * 2.0;
            }
        v[idx].xyz += acc * elapsed_time;
        p[idx].pos.xyz += v[idx].xyz * elapsed_time;
       // p[idx].color.xyz = mix(vec3(0, 0, 1), vec3(1, 0, 0), length(normalize(acc)));
    }
    else
    {
        for (int i = 1; i < N; i += 2)
            {
                vec3 diff = p[idx].pos.xyz - p[i].pos.xyz;
                float distSqr = dot(diff, diff) + 0.001;
                float distSixth = distSqr * distSqr * distSqr;
                float invDistCube = 1.0f/sqrt(distSixth);
                acc -= diff * 40 * invDistCube;
                // Clamping for when particles are too close
                if (length(acc) > 2.0)
                    acc = normalize(acc) * 2.0;
            }
        v[idx].xyz += acc * elapsed_time;
        p[idx].pos.xyz += v[idx].xyz * elapsed_time;
    //    p[idx].color.xyz = mix(vec3(0, 0, 1), vec3(1, 0, 0), length(normalize(acc)));
    }
}
)SHADER";

// Even though we only use the xyz components, we use a vec4 because the std430 layout requires alignment
struct vertex
{
    glm::vec4 pos;
    glm::vec4 color;
};

// Yes, this is one gigantic main function, and it's not "object oriented", but it does the job.
int main()
{
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 2;

    sf::Window window{sf::VideoMode{1280, 1024}, "OpenGL", sf::Style::Close, settings};


    glewExperimental = GL_TRUE;
    glewInit();


    // 0: vertex position and color
    // 1: vertex velocity
    // Velocity is in a different SSBO, because the rendering program doesn't need it
    GLuint ssbos[2];
    glGenBuffers(1, ssbos);

    // Initialize positions and colors randomly
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, num_particles * sizeof (vertex),
                 NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbos[0]);
    vertex* positions = (vertex*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
                                                        0, num_particles * sizeof (vertex),
                                                        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    for (size_t i = 0; i < num_particles; ++i)
    {
        positions[i].pos = glm::vec4(glm::gaussRand(glm::vec3(0), glm::vec3(0.7)), 0.f);
        //positions[i].pos = glm::vec4(glm::sphericalRand(1.f), 0.f);
        positions[i].color = glm::vec4(0.34, 0.28, 0.17, 1.0);
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);


    // Initialize velocity
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, num_particles * sizeof (glm::vec4),
                 NULL, GL_STATIC_DRAW);
    glm::vec4* velocities = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
                                                        0, num_particles * sizeof (glm::vec4),
                                                        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    for (size_t i = 0; i < num_particles; ++i)
        velocities[i] = glm::vec4(glm::gaussRand(glm::vec3(0), glm::vec3(1)), 0);
        //velocities[i] = glm::vec4(0, 0, 0, 0);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);


    // Compiling the vertex shader
    GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v_shader, 1, &vertex_shader, NULL);
    glCompileShader(v_shader);

    // Error checking
    GLint status;
    glGetShaderiv(v_shader, GL_COMPILE_STATUS, &status);
    //std::cout << status << std::endl;
    char buffer[512];
    glGetShaderInfoLog(v_shader, 512, NULL, buffer);
    std::cout << buffer << std::endl;

    // Compiling the geometry shader
    GLuint g_shader = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(g_shader, 1, &geometry_shader, NULL);
    glCompileShader(g_shader);

    glGetShaderInfoLog(g_shader, 512, NULL, buffer);
    std::cout << buffer << std::endl;

    // Compiling the fragment shader
    GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f_shader, 1, &fragment_shader, NULL);
    glCompileShader(f_shader);

    glGetShaderInfoLog(f_shader, 512, NULL, buffer);
    std::cout << buffer << std::endl;

    // Error checking
    glGetShaderiv(f_shader, GL_COMPILE_STATUS, &status);
    std::cout << status << std::endl;

    // If error occured, print the compiling log
    glGetShaderInfoLog(f_shader, 512, NULL, buffer);
    std::cout << buffer << std::endl;

    GLuint render_program = glCreateProgram();
    glAttachShader(render_program, v_shader);
    glAttachShader(render_program, g_shader);
    glAttachShader(render_program, f_shader);

    // Specifying the "output variable" for the fragment shader
    glBindFragDataLocation(render_program, 0, "outColor");

    // Everything Specified, time to link!
    glLinkProgram(render_program);
    glGetProgramInfoLog(render_program, 512, NULL, buffer);
    std::cout << buffer << std::endl;

    /****************************************************/
    // COMPUTE PROGRAM
    /****************************************************/

    GLuint c_shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(c_shader, 1, &compute_shader, NULL);
    glCompileShader(c_shader);

    glGetShaderiv(f_shader, GL_COMPILE_STATUS, &status);
    std::cout << status << std::endl;

    // If error occured, print the compiling log
    glGetShaderInfoLog(v_shader, 512, NULL, buffer);
    std::cout << buffer << std::endl;

    GLuint compute_program = glCreateProgram();
    glAttachShader(compute_program, c_shader);
    glLinkProgram(compute_program);
    glGetProgramInfoLog(compute_program, 512, NULL, buffer);
    std::cout << buffer << std::endl;



    // Initialize VAO
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glUseProgram(render_program);
    // Specifying how to access attributes from buffer object
    glBindBuffer(GL_ARRAY_BUFFER, ssbos[0]);

    GLint attrib_pos = glGetAttribLocation(render_program, "position");
    GLint attrib_color = glGetAttribLocation(render_program, "color");
    std::cout << attrib_color << std::endl;

    glEnableVertexAttribArray(attrib_pos);
    if (auto error = glGetError())
    {
        std::cout << error << std::endl;
        window.close();
    }
    glEnableVertexAttribArray(attrib_color);

    glVertexAttribPointer(attrib_pos, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float), 0);
    glVertexAttribPointer(attrib_color, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(4*sizeof(float)));

    // Initialize render program uniforms
    glm::mat4 projection = glm::perspective(45.f, 800.f / 600.f, 1.f, 10.f);
    GLint uni_proj = glGetUniformLocation(render_program, "projection");
    glUniformMatrix4fv(uni_proj, 1, GL_FALSE, glm::value_ptr(projection));



    glm::mat4 view = glm::lookAt(
            glm::vec3(5.f, 0.f, 0.f),
            glm::vec3(0.f, 0.f, 0.f),
            glm::vec3(0.f, 0.f, 1.f));

    GLint uni_view = glGetUniformLocation(render_program, "view");
    glUniformMatrix4fv(uni_view, 1, GL_FALSE, glm::value_ptr(view));
    sf::Clock clock;

    // Shiny!
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthFunc(GL_ALWAYS);

    int flip = 0;
    while (window.isOpen())
    {

        sf::Time elapsed = clock.restart();

        std::cout << 1.f / elapsed.asSeconds() << std::endl;

        glUseProgram(compute_program);
        GLint uni_time = glGetUniformLocation(compute_program, "elapsed_time");
        glUniform1f(uni_time, elapsed.asSeconds());
        flip != flip;
        GLint uni_flip = glGetUniformLocation(compute_program, "flip");
        glUniform1i(uni_flip, flip);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbos[0]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbos[1]);
        glDispatchCompute((num_particles / 256) + 1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        // RENDER TIME
        glUseProgram(render_program);
        view = glm::rotate(view, elapsed.asSeconds(), glm::vec3(0.f, 0.f, 1.f));
        GLint uni_view = glGetUniformLocation(render_program, "view");
        //glUniformMatrix4fv(uni_view, 1, GL_FALSE, glm::value_ptr(view));
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnableClientState(GL_VERTEX_ARRAY);
        glDrawArrays(GL_POINTS, 0, num_particles);
        glDisableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        window.display();

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }
    }
}
