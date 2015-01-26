#include <vector>
#include <iostream>
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

const size_t num_vertices = 10000;

const GLchar* vertex_shader = R"SHADER(
#version 150 core

in vec2 position;
in vec3 color;

out vec3 Color;

void main()
{
    Color = color;
    gl_Position = vec4(position, 0.0, 1.0);
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
    glm::vec2 pos;
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
	vertex.pos = glm::gaussRand(glm::vec2(0), glm::vec2(0.7));
	vertex.color = glm::linearRand(glm::vec3(0), glm::vec3(1));
    }

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof (vertex), vertices.data(), GL_STATIC_DRAW);

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
    glVertexAttribPointer(attrib_pos, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), 0);
    
    GLint attrib_color = glGetAttribLocation(program, "color");
    glEnableVertexAttribArray(attrib_color);
    glVertexAttribPointer(attrib_color, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(2*sizeof(float)));

    while (window.isOpen())
    {

	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_POINTS, 0, num_vertices);

	window.display();

	sf::Event event;
	while (window.pollEvent(event))
	{
	    if (event.type == sf::Event::Closed)
		window.close();
	}
    }
}
