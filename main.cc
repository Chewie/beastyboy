#define GLM_FORCE_RADIANS
#include <vector>
#include <iostream>
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const size_t num_vertices = 100000;

const GLchar* vertex_shader = R"SHADER(
#version 150 core

in vec3 position;
in vec3 color;

uniform mat4 view;
uniform mat4 projection;

out vec3 Color;

void main()
{
    Color = color;
    gl_Position = projection * view * vec4(position, 1.0);
}
)SHADER";

const GLchar* fragment_shader = R"SHADER(
#version 150 core

in vec3 Color;
out vec4 outColor;

void main()
{
    outColor = vec4(Color, 1.0);
}
)SHADER";

struct vertex
{
    glm::vec3 pos;
    glm::vec3 color;
};

int main()
{
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 2;

    sf::Window window{sf::VideoMode{800, 600}, "OpenGL", sf::Style::Close, settings};


    glewExperimental = GL_TRUE;
    glewInit();


    std::vector<vertex> vertices(num_vertices);

    for (auto& vertex : vertices)
    {
	vertex.pos = glm::gaussRand(glm::vec3(0), glm::vec3(0.7));
	vertex.color = glm::linearRand(glm::vec3(0), glm::vec3(1));
    }

    std::vector<glm::vec3> velocities(num_vertices);
    for (auto& v : velocities)
	v = glm::linearRand(glm::vec3(-1), glm::vec3(1));

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof (vertex), vertices.data(), GL_DYNAMIC_DRAW);

    GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v_shader, 1, &vertex_shader, NULL);
    glCompileShader(v_shader);

    GLint status;
    glGetShaderiv(v_shader, GL_COMPILE_STATUS, &status);
    std::cout << status << std::endl;

    GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f_shader, 1, &fragment_shader, NULL);
    glCompileShader(f_shader);

    glGetShaderiv(f_shader, GL_COMPILE_STATUS, &status);
    std::cout << status << std::endl;

    char buffer[512];
    glGetShaderInfoLog(v_shader, 512, NULL, buffer);
    std::cout << buffer << std::endl;

    GLuint program = glCreateProgram();
    glAttachShader(program, v_shader);
    glAttachShader(program, f_shader);

    glBindFragDataLocation(program, 0, "outColor");

    glLinkProgram(program);
    glUseProgram(program);

    GLint attrib_pos = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(attrib_pos);
    glVertexAttribPointer(attrib_pos, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), 0);
    
    GLint attrib_color = glGetAttribLocation(program, "color");
    glEnableVertexAttribArray(attrib_color);
    glVertexAttribPointer(attrib_color, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));

    glm::mat4 projection = glm::perspective(45.f, 800.f / 600.f, 1.f, 10.f);
    GLint uni_proj = glGetUniformLocation(program, "projection");
    glUniformMatrix4fv(uni_proj, 1, GL_FALSE, glm::value_ptr(projection));

    glm::mat4 view = glm::lookAt(
	    glm::vec3(5.f, 0.f, 0.f),
	    glm::vec3(0.f, 0.f, 0.f),
	    glm::vec3(0.f, 0.f, 1.f));

    sf::Clock clock;
    while (window.isOpen())
    {
	sf::Time elapsed = clock.restart();
	view = glm::rotate(view, elapsed.asSeconds(), glm::vec3(0.f, 0.f, 1.f));
	GLint uni_view = glGetUniformLocation(program, "view");
	glUniformMatrix4fv(uni_view, 1, GL_FALSE, glm::value_ptr(view));

	std::cout << 1.f / elapsed.asSeconds() << std::endl;

	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_POINTS, 0, num_vertices);

	window.display();

	for (size_t i = 0; i < vertices.size(); ++i)
	{
	    vertices[i].pos = vertices[i].pos + velocities[i] * elapsed.asSeconds();
	    if (vertices[i].pos.x > 1.3f)
	    {
		vertices[i].pos.x = 1.3f;
		velocities[i].x *= -1;
	    }
	    if (vertices[i].pos.x < -1.3f)
	    {
		vertices[i].pos.x = 1.3f;
		velocities[i].x *= -1;
	    }
	    if (vertices[i].pos.y > 1.3f)
	    {
		vertices[i].pos.y = 1.3f;
		velocities[i].y *= -1;
	    }
	    if (vertices[i].pos.y < -1.3f)
	    {
		vertices[i].pos.y = 1.3f;
		velocities[i].y *= -1;
	    }
	    if (vertices[i].pos.z > 1.3f)
	    {
		vertices[i].pos.z = 1.3f;
		velocities[i].z *= -1;
	    }
	    if (vertices[i].pos.z < -1.3f)
	    {
		vertices[i].pos.z = 1.3f;
		velocities[i].z *= -1;
	    }
	}

	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof (vertex), vertices.data());
	sf::Event event;
	while (window.pollEvent(event))
	{
	    if (event.type == sf::Event::Closed)
		window.close();
	}
    }
}
