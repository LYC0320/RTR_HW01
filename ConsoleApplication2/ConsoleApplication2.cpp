#include "stdafx.h"
#include "iostream"
#include "../GL/glew.h"
#include "GL/glut.h"
#include "../glm/glm/glm.hpp"
#include "vector"
#include <../glm/glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <sstream>
#include <math.h>


#pragma warning( disable : 4996 )

using namespace glm;
using namespace std;

std::vector<glm::vec3> vertices;
// indices需unsigned int
std::vector<unsigned int> indices;
GLuint shaderProgram;
GLuint VBO, VAO, EBO;
glm::mat4 MVP;
glm::mat4 finalMVP;

vec3 camPos;
vec3 camLook;
vec3 camUp;

float fov;
float aspect;
float nearDis;
float farDis;

glm::mat4 V, P;
glm::mat4 M(1.0f);

glm::mat4 radius(1.0f);

float frame = 0;

std::string vertexShaderSource2;

const GLchar* GLVSS2;

const GLchar* fragmentShaderSource = "#version 330 core\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
"}\n\0";

void readShader(std::string path)
{
	//std::vector<std::string> line;
	std::string lineBuffer;
	std::string buffer;


	std::ifstream infile;
	infile = std::ifstream(path);

	if (infile.bad()) {
		std::cout << path << " does not exists." << std::endl;
		return;
	}
	else {
		std::cout << path << " is OK." << std::endl;
	}

	while (std::getline(infile, buffer))
	{
		lineBuffer += buffer + "\n";
	}

	vertexShaderSource2 = lineBuffer;
}

void loadOBJ(char *path, std::vector<glm::vec3> &vertices)
{
	FILE *file = fopen(path, "r");
	if (file == NULL)
	{
		printf("Impossible to open the file !\n");
	}

	while (1)
	{
		char lineHeader[128];

		int res = fscanf(file, "%s", lineHeader);

		if (res == EOF)
			break;

		if (strcmp(lineHeader, "v") == 0)
		{
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "f") == 0)
		{
			int x, y, z;
			fscanf(file, "%d %d %d", &x, &y, &z);
			indices.push_back(x - 1);
			indices.push_back(y - 1);
			indices.push_back(z - 1);
		}
	}
}

void initShader(void)
{
	glewInit();

	// vertexShader
	GLuint vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);

	GLVSS2 = vertexShaderSource2.c_str();

	glShaderSource(vertexShader, 1, &GLVSS2, NULL);
	glCompileShader(vertexShader);

	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	// fragmentShader
	GLuint fragmentShader;
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	// shaderProgram
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::LINK::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	// deleteShader
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// GenBuffer
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// VAO需先bind
	glBindVertexArray(VAO);

	// VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	// EBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);

	// VAO	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Unbind Buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void setupCamera(void)
{
	camPos = vec3(0.0, 0.0, 15.0);
	camLook = vec3(0.0, 0.0, -1.0);
	camUp = vec3(0, 1, 0);

	fov = 45.0;
	aspect = (float)1 / (float)2;
	nearDis = 0.1;
	farDis = 1000;

	radius = glm::translate(radius, vec3(2.0, 0, 0));

	V = glm::lookAt(camPos, camPos + camLook, camUp);
	P = glm::perspective(glm::radians(fov), aspect, nearDis, farDis);
	MVP = P*V*M;
}

vec4 quaternion(float angle, vec3 axis)
{
	vec4 q;

	q.x = axis.x*sin(angle*0.5);
	q.y = axis.y*sin(angle*0.5);
	q.z = axis.z*sin(angle*0.5);
	q.w = cos(angle*0.5);

	return q;
}

mat4 quatnionToMatrix(vec4 q)
{
	mat4 m;

	m[0][0] = 1 - 2 * q.z*q.z - 2 * q.y * q.y;
	m[0][1] = -2 * q.z * q.w + 2 * q.y * q.x;
	m[0][2] = 2 * q.y * q.w + 2 * q.z * q.x;
	m[0][3] = 0;

	m[1][0] = 2 * q.x * q.y + 2 * q.z * q.w;
	m[1][1] = 1 - 2 * q.x * q.x - 2 * q.z * q.z;
	m[1][2] = 2 * q.y * q.z - 2 * q.x * q.w;
	m[1][3] = 0;

	m[2][0] = 2 * q.x * q.z - 2 * q.y * q.w;
	m[2][1] = 2 * q.y * q.z + 2 * q.x * q.w;
	m[2][2] = 1 - 2 * q.x * q.x - 2 * q.y * q.y;
	m[2][3] = 0;

	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 0;
	m[3][3] = 1;

	return m;
}

void update(void)
{
	finalMVP = MVP * quatnionToMatrix(quaternion(frame, vec3(0.0, 1.0, 0.0))) * radius * quatnionToMatrix(quaternion(frame, vec3(1.0, 0.0, 0.0)));
}


void Display(void)
{
	frame = frame + 0.05f;

	update();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shaderProgram);
	glBindVertexArray(VAO);
	//glDrawArrays(GL_TRIANGLES, 0, vertices.size());

	// uniform MVP before render
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"), 1, GL_FALSE, &finalMVP[0][0]);

	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glutSwapBuffers();

	glutPostRedisplay();
}

int main(int argc, char *argv[])
{
	loadOBJ("../teapot.obj", vertices);


	setupCamera();
	readShader("../Shader.txt");

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowPosition(150, 200); // 設定?窗位置 
	glutInitWindowSize(400, 400); // 設定?窗大小 
	glutCreateWindow("RTR_HW01_0656632"); // 設定?窗標題

	glutDisplayFunc(Display);

	initShader();

	glutMainLoop();

	return 0;
}
