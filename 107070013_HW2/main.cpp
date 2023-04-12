#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define PI 3.1415926

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;

int window_width = 800;
int window_height = 800;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	LightEdit = 3,
	ShininessEdit = 4,
};

struct Uniform
{
	GLint iLocMVP;
	GLint iLocKa;
	GLint iLocKd;
	GLint iLocKs;
	GLint iLocmv;
	GLint iLocView;
	GLint iLocCamera;
	GLint iLocShininess;


	GLint iLocI_d;
	GLint iLocI_p;
	GLint iLocI_s;


	GLint iLocPos_d;
	GLint iLocPos_p;
	GLint iLocPos_s;


	GLuint iLocper_vertex;
	GLuint iLocLight_id;
	GLuint iLocSpot_cutoff;

};
Uniform uniform;

//Diffuse
Vector3 I_d = Vector3(1.0f, 1.0f, 1.0f);
Vector3 I_p = Vector3(1.0f, 1.0f, 1.0f);
Vector3 I_s = Vector3(1.0f, 1.0f, 1.0f);

//Light Position
Vector3 lightPos_d = Vector3(1.0f, 1.0f, 1.0f);
Vector3 lightPos_p = Vector3(0.0f, 2.0f, 1.0f);
Vector3 lightPos_s = Vector3(0.0f, 0.0f, 2.0f);

vector<string> filenames; // .obj filename list

struct PhongMaterial
{
	Vector3 Ka;
	Vector3 Kd;
	Vector3 Ks;
};

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	PhongMaterial material;
	int indexCount;
	GLuint m_texture;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form

	vector<Shape> shapes;
};
vector<model> models;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

TransMode cur_trans_mode = GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;

int cur_idx = 0; // represent which model should be rendered now
int cur_light_id = 0; //0:directional,1:point,2:spot
int per_vertex = 0; //per-vertex shading

float shininess = 64.0f;
float spot_cutoff = 30;


static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}


// [DONE] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		1, 0, 0, vec.x,
		0, 1, 0, vec.y,
		0, 0, 1, vec.z,
		0, 0, 0, 1
	);

	return mat;
}

// [DONE] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		vec.x, 0, 0, 0,
		0, vec.y, 0, 0,
		0, 0, vec.z, 0,
		0, 0, 0, 1
	);

	return mat;
}


// [DONE] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;

	GLfloat c_v = cos(val*PI / 180.0f); //cos value
	GLfloat s_v = sin(val*PI / 180.0f); //sin value
	mat = Matrix4(
		1, 0, 0, 0,
		0, c_v, -s_v, 0,
		0, s_v, c_v, 0,
		0, 0, 0, 1
	);

	return mat;
}

// [DONE] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;

	GLfloat c_v = cos(val*PI / 180.0f); //cos value
	GLfloat s_v = sin(val*PI / 180.0f); //sin value
	mat = Matrix4(
		c_v, 0, s_v, 0,
		0, 1, 0, 0,
		-s_v, 0, c_v, 0,
		0, 0, 0, 1
	);

	return mat;
}

// [DONE] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;

	GLfloat c_v = cos(val*PI / 180.0f); //cos value
	GLfloat s_v = sin(val*PI / 180.0f); //sin value
	mat = Matrix4(
		c_v, -s_v, 0, 0,
		s_v, c_v, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);

	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [DONE] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
	Vector3 r_x, r_y, r_z;
	Vector3 p_12, p_13;
	p_12 = main_camera.center - main_camera.position; //f
	p_13 = main_camera.up_vector.normalize(); //u'
	r_z = p_12.normalize(); //f'
	r_x = r_z.cross(p_13);  //s
	r_y = r_x.cross(p_12);  // u''


	Matrix4 R, T;

	R = Matrix4(
		r_x.x, r_x.y, r_x.z, 0,
		r_y.x, r_y.y, r_y.z, 0,
		(-r_z.x), (-r_z.y), (-r_z.z), 0,
		0, 0, 0, 1
	);
	//cout << R << endl;

	T = Matrix4(
		1, 0, 0, -main_camera.position.x,
		0, 1, 0, -main_camera.position.y,
		0, 0, 1, -main_camera.position.z,
		0, 0, 0, 1
	);
	//cout << T << endl;

	view_matrix = R * T;
	//cout << view_matrix << endl;
}

// [DONE] compute persepective projection matrix
void setPerspective()
{
	// GLfloat f = ...
	// project_matrix [...] = ...
	GLfloat f = 1 / tan(((proj.fovy) / 2) * PI / 180.0f);
	if (proj.aspect < 1) {
		f = f * proj.aspect;
		project_matrix = Matrix4(
			(f / proj.aspect), 0, 0, 0,
			0, f, 0, 0,
			0, 0, (proj.farClip + proj.nearClip) / (proj.nearClip - proj.farClip), (2 * proj.farClip*proj.nearClip) / (proj.nearClip - proj.farClip),
			0, 0, -1, 0
		);
	}
	else {
		project_matrix = Matrix4(
			(f / proj.aspect), 0, 0, 0,
			0, f, 0, 0,
			0, 0, (proj.farClip + proj.nearClip) / (proj.nearClip - proj.farClip), (2 * proj.farClip*proj.nearClip) / (proj.nearClip - proj.farClip),
			0, 0, -1, 0
		);
	}
}

void setGLMatrix(GLfloat* glm, Matrix4& m) {
	glm[0] = m[0];  glm[4] = m[1];   glm[8] = m[2];    glm[12] = m[3];
	glm[1] = m[4];  glm[5] = m[5];   glm[9] = m[6];    glm[13] = m[7];
	glm[2] = m[8];  glm[6] = m[9];   glm[10] = m[10];   glm[14] = m[11];
	glm[3] = m[12];  glm[7] = m[13];  glm[11] = m[14];   glm[15] = m[15];
}

// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	window_width = width;
	window_height = height;
	// [TODO] change your aspect ratio
	proj.aspect = (float)(width/2) / (float)height;
	setPerspective();
}



// Render function for display rendering
void RenderScene(void) {
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Matrix4 T, R, S;
	// [DONE] update translation, rotation and scaling
	T = translate(models[cur_idx].position);
	R = rotate(models[cur_idx].rotation);
	S = scaling(models[cur_idx].scale);

	Matrix4 MVP;
	Matrix4 mv;
	GLfloat mvp[16];

	// [DONE] multiply all the matrix
	MVP = project_matrix * view_matrix * T * R * S;

	//mv = view_matrix * T * R * S;
	mv = T * R * S;
	// row-major ---> column-major
	setGLMatrix(mvp, MVP);

	// use uniform to send mvp to vertex shader
	glUniformMatrix4fv(uniform.iLocMVP, 1, GL_FALSE, mvp);
	glUniformMatrix4fv(uniform.iLocmv, 1, GL_FALSE, mv.getTranspose());
	glUniformMatrix4fv(uniform.iLocView, 1, GL_FALSE, view_matrix.getTranspose());
	glUniform3f(uniform.iLocCamera, main_camera.position.x, main_camera.position.y, main_camera.position.z);

	glUniform3f(uniform.iLocI_d, I_d.x, I_d.y, I_d.z);
	glUniform3f(uniform.iLocI_p, I_p.x, I_p.y, I_p.z);
	glUniform3f(uniform.iLocI_s, I_s.x, I_s.y, I_s.z);

	glUniform3f(uniform.iLocPos_d, lightPos_d.x, lightPos_d.y, lightPos_d.z);
	glUniform3f(uniform.iLocPos_p, lightPos_p.x, lightPos_p.y, lightPos_p.z);
	glUniform3f(uniform.iLocPos_s, lightPos_s.x, lightPos_s.y, lightPos_s.z);

	glUniform1f(uniform.iLocShininess, shininess);
	glUniform1i(uniform.iLocLight_id, cur_light_id);
	glUniform1f(uniform.iLocSpot_cutoff, spot_cutoff);

	for (int i = 0; i < models[cur_idx].shapes.size(); i++)
	{
		// set glViewport and draw twice ... 
		per_vertex = 1;
		glUniform1i(uniform.iLocper_vertex, per_vertex);
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
		glUniform3fv(uniform.iLocKa, 1, &(models[cur_idx].shapes[i].material.Ka[0]));
		glUniform3fv(uniform.iLocKd, 1, &(models[cur_idx].shapes[i].material.Kd[0]));
		glUniform3fv(uniform.iLocKs, 1, &(models[cur_idx].shapes[i].material.Ks[0]));


	}
	glViewport(0, 0, window_width / 2, window_height);

	for (int i = 0; i < models[cur_idx].shapes.size(); i++)
	{
		// set glViewport and draw twice ...
		per_vertex = 0;
		glUniform1i(uniform.iLocper_vertex, per_vertex);
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
		glUniform3fv(uniform.iLocKa, 1, &(models[cur_idx].shapes[i].material.Ka[0]));
		glUniform3fv(uniform.iLocKd, 1, &(models[cur_idx].shapes[i].material.Kd[0]));
		glUniform3fv(uniform.iLocKs, 1, &(models[cur_idx].shapes[i].material.Ks[0]));


	}
	glViewport(window_width / 2, 0, window_width / 2, window_height);

}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// [TODO] Call back function for keyboard
	if (key == GLFW_KEY_Z && action == GLFW_PRESS) {/* switch pre model */
		cur_idx -= 1;
		if (cur_idx < 0) {
			cur_idx = 4;
		}
	}
	else if (key == GLFW_KEY_X && action == GLFW_PRESS) {/* switch post model */
		cur_idx += 1;
		if (cur_idx > 4) {
			cur_idx = 0;
		}
	}
	else if (key == GLFW_KEY_T && action == GLFW_PRESS) {/* translation mode */
		cur_trans_mode = GeoTranslation;
	}
	else if (key == GLFW_KEY_S && action == GLFW_PRESS) {/* scale mode */
		cur_trans_mode = GeoScaling;
	}
	else if (key == GLFW_KEY_R && action == GLFW_PRESS) {/* rotation mode */
		cur_trans_mode = GeoRotation;
	}
	else if (key == GLFW_KEY_L && action == GLFW_PRESS) {/* switch between directional/point/spot light */
		cur_light_id += 1;
		if (cur_light_id > 2) {
			cur_light_id = 0;
		}
		cout << cur_light_id << endl;
	}
	else if (key == GLFW_KEY_K && action == GLFW_PRESS) {/* switch to light editing mode*/
		cur_trans_mode = LightEdit;
	}
	else if (key == GLFW_KEY_J && action == GLFW_PRESS) {/* switch to shininess editinig mode */
		cur_trans_mode = ShininessEdit;
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// [TODO] scroll up positive, otherwise it would be negtive
	if (cur_trans_mode == GeoTranslation) {
		models[cur_idx].position.z += 0.5*yoffset;
	}
	else if (cur_trans_mode == GeoRotation) {
		models[cur_idx].rotation.z += 0.5*yoffset;
	}
	else if (cur_trans_mode == GeoScaling) {
		models[cur_idx].scale.z += 0.5*yoffset;
	}
	else if (cur_trans_mode == LightEdit) {
		if (cur_light_id == 0)
		{
			I_d = I_d + Vector3(1, 1, 1)*0.5*yoffset;
		}
		else if (cur_light_id == 1)
		{
			I_p = I_p + Vector3(1, 1, 1)*0.5*yoffset;
		}
		else if (cur_light_id == 2)
		{
			spot_cutoff += yoffset;
		}
	}
	else if (cur_trans_mode == ShininessEdit) {
		shininess += 1.5*yoffset;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// [DONE] mouse press callback function
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		mouse_pressed = true;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		mouse_pressed = false;
	}

}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	// [TODO] cursor position callback function
	float xoffset, yoffset;
	xoffset = xpos - starting_press_x;
	yoffset = starting_press_y - ypos;
	starting_press_x = xpos;
	starting_press_y = ypos;

	if (mouse_pressed) {
		if (cur_trans_mode == GeoTranslation) {
			models[cur_idx].position.x += 0.01*xoffset;
			models[cur_idx].position.y += 0.01*yoffset;
		}
		else if (cur_trans_mode == GeoRotation) {
			models[cur_idx].rotation.y += 5 * PI / 180 * xoffset;
			models[cur_idx].rotation.x += 5 * PI / 180 * yoffset;
		}
		else if (cur_trans_mode == GeoScaling) {
			models[cur_idx].scale.x += 0.01*xoffset;
			models[cur_idx].scale.y += 0.01*yoffset;
		}
		else if (cur_trans_mode == LightEdit) {
			if (cur_light_id == 0)
			{
				lightPos_d.x += 0.01*xoffset;
				lightPos_d.y += 0.01*yoffset;
			}
			else if (cur_light_id == 1)
			{
				lightPos_p.x += 0.01*xoffset;
				lightPos_p.y += 0.01*yoffset;
			}
			else if (cur_light_id == 2)
			{
				lightPos_s.x += 0.01*xoffset;
				lightPos_s.y += 0.01*yoffset;
			}
		}
	}
}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p, f);
	glAttachShader(p, v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	uniform.iLocMVP = glGetUniformLocation(p, "mvp");
	uniform.iLocKa = glGetUniformLocation(p, "Ka");
	uniform.iLocKd = glGetUniformLocation(p, "Kd");
	uniform.iLocKs = glGetUniformLocation(p, "Ks");
	uniform.iLocmv = glGetUniformLocation(p, "mv");
	uniform.iLocView = glGetUniformLocation(p, "view_matrix");
	uniform.iLocCamera = glGetUniformLocation(p, "cameraPos");
	uniform.iLocShininess = glGetUniformLocation(p, "shininess");

	uniform.iLocI_d = glGetUniformLocation(p, "I_d");
	uniform.iLocI_p = glGetUniformLocation(p, "I_p");
	uniform.iLocI_s = glGetUniformLocation(p, "I_s");

	uniform.iLocPos_d = glGetUniformLocation(p, "lightPos_d");
	uniform.iLocPos_p = glGetUniformLocation(p, "lightPos_p");
	uniform.iLocPos_s = glGetUniformLocation(p, "lightPos_s");

	uniform.iLocper_vertex = glGetUniformLocation(p, "per_vertex");
	uniform.iLocLight_id = glGetUniformLocation(p, "cur_light_id");
	uniform.iLocSpot_cutoff = glGetUniformLocation(p, "spot_cutoff");


	if (success)
		glUseProgram(p);
	else
	{
		system("pause");
		exit(123);
	}
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			if (idx.normal_index >= 0) {
				normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			}
		}
		index_offset += fv;
	}
}

string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;

	string err;
	string warn;

	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Material size %d\n", int(shapes.size()), int(materials.size()));
	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		allMaterial.push_back(material);
	}

	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		normalization(&attrib, vertices, colors, normals, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);

		Shape tmp_shape;
		glGenVertexArrays(1, &tmp_shape.vao);
		glBindVertexArray(tmp_shape.vao);

		glGenBuffers(1, &tmp_shape.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		tmp_shape.vertex_count = vertices.size() / 3;

		glGenBuffers(1, &tmp_shape.p_color);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &tmp_shape.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// not support per face material, use material of first face
		if (allMaterial.size() > 0)
			tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
		tmp_model.shapes.push_back(tmp_shape);
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	// [TODO] Setup some parameters if you need
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)WINDOW_WIDTH/2 / (float)WINDOW_HEIGHT;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj" };
	// [DONE] Load five model at here
	for (int i = 0; i <= 4; i++) {
		LoadModels(model_list[i]);
	}
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}


int main(int argc, char **argv)
{
	// initial glfw
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif


	// create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "107070013_HW2", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);


	// load OpenGL function pointer
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// register glfw callback functions
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

	glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();

	// main loop
	while (!glfwWindowShouldClose(window))
	{
		// render
		RenderScene();

		// swap buffer from back to front
		glfwSwapBuffers(window);

		// Poll input event
		glfwPollEvents();
	}

	// just for compatibiliy purposes
	return 0;
}
