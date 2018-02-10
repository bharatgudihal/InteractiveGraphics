// InteractiveGraphics.cpp : Defines the entry point for the console application.
//

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cyTriMesh.h>
#include <cyGL.h>
#include <cyMatrix.h>
#include <lodepng.h>
#include <iostream>
#include <string>

struct {
	float r, g, b, a;
}color;

struct VertexData{
	cyPoint3f vertexPosition;
	cyPoint3f vertexNormal;
	cyPoint2f uv;
};

struct Material {
	cyPoint3f ambient;
	cyPoint3f diffuse;
	cyPoint3f specular;
	float shininess;
};

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float FOV = 0.785398f;
const float NEAR_PLANE = 0.1f;
const float FAR_PLANE = 1000.0f;
cyTriMesh mesh;
GLuint VAO, VBO, diffuseTextureId, specularTextureId;
cyGLSLProgram program;
cyMatrix4f mvp;
cyMatrix4f model = cyMatrix4f::MatrixTrans(cyPoint3f(0, 0, 0));
cyMatrix4f cameraRotation = cyMatrix4f::MatrixRotationX(0);
cyMatrix4f cameraTranslation = cyMatrix4f::MatrixTrans(cyPoint3f(0,0,-70.0f));
bool isProjection = true;
cyMatrix4f projection = cyMatrix4f::MatrixPerspective(FOV, WINDOW_WIDTH / (float)WINDOW_HEIGHT, NEAR_PLANE, FAR_PLANE);
float lastRightMousePos;
float lastLeftMousePosX;
float lastLeftMousePosY;
float rotationSpeed = 0.0174533f;
float xRot = 0.0f;
float yRot = 0.0f;
int pressedButton;
cyGLSLShader vertexShader;	
cyGLSLShader fragmentShader;
Material material;
cyPoint3f lightPosition(70.0f, 70.0f, 70.0f);
bool isLightRotating;
float xLightRot = 0.0f;
float yLightRot = 0.0f;

void CompileShaders();
void Display();
void Idle();
void GetKeyboardInput(unsigned char key, int xmouse, int ymouse);
void GetKeySpecial(int key, int xmouse, int ymouse);
void GetKeyUpSpecial(int key, int xmouse, int ymouse);
void GetMouseInput(int button, int state, int xmouse, int ymouse);
void GetMousePosition(int x, int y);
void TogglePerspective();

int main(int argc, char *argv[])
{	
	
	color = { 0.0f,0.0f,0.0f,0.0f };

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("Test");
	glutDisplayFunc(Display);
	glutIdleFunc(Idle);
	glutKeyboardFunc(GetKeyboardInput);
	glutMouseFunc(GetMouseInput);
	glutMotionFunc(GetMousePosition);
	glutSpecialFunc(GetKeySpecial);
	glutSpecialUpFunc(GetKeyUpSpecial);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glewInit();

	//Mesh
	char* filePath;
	if (argc > 1) {		
		filePath = argv[1];
		std::cout << "Loading file " << filePath << std::endl;
	}
	else {
		filePath = "Assets/Models/teapot/teapot.obj";
	}
	std::string parentFolder(filePath);
	parentFolder = parentFolder.substr(0, parentFolder.find_last_of("/")+1);
	if (mesh.LoadFromFileObj(filePath)) {
		
		mesh.ComputeBoundingBox();
		//Adjust model to accomodate bounding box at 0,0,0
		model = cyMatrix4f::MatrixRotationX(-1.5708f) * cyMatrix4f::MatrixTrans(-(mesh.GetBoundMax() + mesh.GetBoundMin()) / 2.0f);
		if (!mesh.HasNormals()) {
			mesh.ComputeNormals();
		}

		//Extract all vertex data
		VertexData* vertices = new VertexData[mesh.NF() * 3];
		int a = mesh.NF();
		int b = mesh.NVT();
		for (unsigned int i = 0; i < mesh.NF(); i++) {
			cyTriMesh::TriFace face = mesh.F(i);
			cyTriMesh::TriFace normalFace = mesh.FN(i);
			cyTriMesh::TriFace textureFace = mesh.FT(i);
			unsigned int vertexIndex = i * 3;
			vertices[vertexIndex].vertexPosition = mesh.V(face.v[0]);
			vertices[vertexIndex + 1].vertexPosition = mesh.V(face.v[1]);
			vertices[vertexIndex + 2].vertexPosition = mesh.V(face.v[2]);

			vertices[vertexIndex].vertexNormal = mesh.VN(normalFace.v[0]);
			vertices[vertexIndex + 1].vertexNormal = mesh.VN(normalFace.v[1]);
			vertices[vertexIndex + 2].vertexNormal = mesh.VN(normalFace.v[2]);

			vertices[vertexIndex].uv = cyPoint2f(mesh.VT(textureFace.v[0]));
			vertices[vertexIndex + 1].uv = cyPoint2f(mesh.VT(textureFace.v[1]));
			vertices[vertexIndex + 2].uv = cyPoint2f(mesh.VT(textureFace.v[2]));
		}
						
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, mesh.NF() * 3 * sizeof(VertexData), vertices, GL_STATIC_DRAW);
		// Position vertex attribute
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, vertexPosition));
		// Position vertex attribute
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, vertexNormal));
		//UV
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, uv));
		CompileShaders();
		delete[] vertices;

		//Get material information		
		cyTriMesh::Mtl mat = mesh.M(0);
		std::vector<unsigned char> imageBuffer;
		std::string diffuseMap = parentFolder + std::string(mat.map_Kd.data);
		unsigned width, height;
		int result = lodepng::decode(imageBuffer, width, height, diffuseMap, LodePNGColorType::LCT_RGB);
		if (result == 0) {
			glGenTextures(1, &diffuseTextureId);
			glBindTexture(GL_TEXTURE_2D, diffuseTextureId);

			// Set texture wrapping params
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			// Set texture filtering params
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageBuffer.data());
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		imageBuffer.clear();
		std::string specularMap = parentFolder + std::string(mat.map_Ks.data);
		result = lodepng::decode(imageBuffer, width, height, specularMap, LodePNGColorType::LCT_RGB);
		if (result == 0) {
			glGenTextures(1, &specularTextureId);
			glBindTexture(GL_TEXTURE_2D, specularTextureId);

			// Set texture wrapping params
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			// Set texture filtering params
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageBuffer.data());
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		material.ambient = cyPoint3f(1.0f, 0.0f, 0.0f);
		material.diffuse = cyPoint3f(1.0f, 0.0f, 0.0f);
		material.specular = cyPoint3f(1.0f, 1.0f, 1.0f);
		material.shininess = 50.0f;

		glutMainLoop();		
	}
	else {
		std::cerr << "File not found" << std::endl;
	}
}

void Display() {	
	glClearColor(color.r, color.g, color.b, color.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	cyMatrix4f view = cameraTranslation * cameraRotation;
	mvp = projection * view * model;
	cyMatrix3f cameraMatrix = (view * model).GetSubMatrix3();
	cyMatrix4f worldToModel = cyMatrix4f::MatrixRotationY(yLightRot) * cyMatrix4f::MatrixRotationX(xLightRot);
	cyMatrix4f lightModelMatrix = worldToModel * cyMatrix4f::MatrixTrans(lightPosition);
	cyPoint3f transformedLightPos = lightModelMatrix.GetTrans();
	
	//Set all shader params
	program.Bind();
	program.SetUniformMatrix4("mvp", mvp.data);	
	program.SetUniformMatrix4("cameraMatrix", (view * model).data);
	cameraMatrix.Invert();
	cameraMatrix.Transpose();
	program.SetUniformMatrix3("normalMatrix", cameraMatrix.data);
	program.SetUniform("lightPosition", transformedLightPos);
	program.SetUniform("ambient", material.ambient);
	program.SetUniform("diffuse", material.diffuse);
	program.SetUniform("specular", material.specular);
	program.SetUniform("shininess", material.shininess);
	cyPoint3f cameraPosition = view.GetTrans();
	program.SetUniform("cameraPosition", cameraPosition);
	
	//Set active texture and bind
	//Diffuse
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuseTextureId);
	
	//Specular
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, specularTextureId);

	//Draw mesh
	glBindVertexArray(VAO);	
	glDrawArrays(GL_TRIANGLES, 0, mesh.NF() * 3);
	glutSwapBuffers();
}

void Idle() {
	glutPostRedisplay();
}

void GetKeyboardInput(unsigned char key, int xmouse, int ymouse) {
	if (key == 27) {
		glutLeaveMainLoop();
	}
	if (key == 112) {
		TogglePerspective();
	}
}

void GetKeySpecial(int key, int xmouse, int ymouse) {
	
	if (key == GLUT_KEY_F6) {
		CompileShaders();
	}

	if (key == GLUT_KEY_CTRL_L || key == GLUT_KEY_CTRL_R) {
		isLightRotating = true;
	}
}

void GetKeyUpSpecial(int key, int xmouse, int ymouse) {

	if (key == GLUT_KEY_CTRL_L || key == GLUT_KEY_CTRL_R) {
		isLightRotating = false;
	}
}

void GetMouseInput(int button, int state, int xmouse, int ymouse) {
	pressedButton = button;
	if (GLUT_LEFT_BUTTON == button) {
		if (state == GLUT_DOWN) {
			lastLeftMousePosX = static_cast<float>(xmouse);
			lastLeftMousePosY = static_cast<float>(ymouse);
		}
		else {
			lastLeftMousePosX = static_cast<float>(xmouse);
			lastLeftMousePosY = static_cast<float>(ymouse);
		}
	}
	else {
		if (state == GLUT_DOWN) {
			lastRightMousePos = static_cast<float>(ymouse);
		}
	}
}

void GetMousePosition(int x, int y) {
	if (pressedButton == GLUT_LEFT_BUTTON) {
		float xDiff = (x - lastLeftMousePosX) * rotationSpeed;
		lastLeftMousePosX = static_cast<float>(x);
		float yDiff = (y - lastLeftMousePosY) * rotationSpeed;
		lastLeftMousePosY = static_cast<float>(y);
		if (isLightRotating) {
			yLightRot += xDiff;
			xLightRot += yDiff;
		}else{			
			yRot += xDiff;
			xRot += yDiff;
			cameraRotation = cameraRotation.MatrixRotationY(yRot) * cameraRotation.MatrixRotationX(xRot);
		}
	}
	else {
		float diff = static_cast<float>(y) - lastRightMousePos;
		lastRightMousePos = static_cast<float>(y);
		cameraTranslation.AddTrans(-cyPoint3f(0.0f, 0.0f, diff));
	}
}

void CompileShaders() {
	vertexShader.CompileFile("Assets/Shaders/Vertex/mesh.glsl", GL_VERTEX_SHADER);
	fragmentShader.CompileFile("Assets/Shaders/Fragment/mesh.glsl", GL_FRAGMENT_SHADER);
	program.Build(&vertexShader, &fragmentShader);
}

void TogglePerspective() {
	if (isProjection) {
		projection = cyMatrix4f::MatrixScale( 1 / cameraTranslation.GetTrans().Length());
	}
	else {
		projection = cyMatrix4f::MatrixPerspective(0.785398f, WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100);
	}
	isProjection = !isProjection;
}