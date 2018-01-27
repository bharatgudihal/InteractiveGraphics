// InteractiveGraphics.cpp : Defines the entry point for the console application.
//

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cyTriMesh.h>
#include <cyGL.h>
#include <cyMatrix.h>

struct {
	float r, g, b, a;
}color;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
cy::TriMesh mesh;
GLuint VAO, VBO;
cy::GLSLProgram program;
cy::Matrix4f mvp;
cy::Matrix4f model = cy::Matrix4f::MatrixTrans(cy::Point3f(0, 0, 0));
cy::Matrix4f cameraRotation = cy::Matrix4f::MatrixRotationX(0);
cy::Matrix4f cameraTranslation = cy::Matrix4f::MatrixTrans(cy::Point3f(0,0,-5.0f));
cy::Matrix4f projection = cy::Matrix4f::MatrixPerspective(0.785398f, WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100);
float lastRightMousePos;
float lastLeftMousePosX;
float lastLeftMousePosY;
float rotationSpeed = 0.0174533f;
float xRot = 0.0f;
float yRot = 0.0f;
int pressedButton;
cy::GLSLShader vertexShader;	
cy::GLSLShader fragmentShader;
void CompileShaders();
void Display();
void Idle();
void GetKeyboardInput(unsigned char key, int xmouse, int ymouse);
void GetKeySpecial(int key, int xmouse, int ymouse);
void GetMouseInput(int button, int state, int xmouse, int ymouse);
void GetMousePosition(int x, int y);

int main(int argc, char *argv)
{
	
	
	color = { 0.0f,0.0f,0.0f,0.0f };

	glutInit(&argc, &argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("Test");
	glutDisplayFunc(Display);
	glutIdleFunc(Idle);
	glutKeyboardFunc(GetKeyboardInput);
	glutMouseFunc(GetMouseInput);
	glutMotionFunc(GetMousePosition);
	glutSpecialFunc(GetKeySpecial);

	//Mesh	
	mesh.LoadFromFileObj("Assets/teapot.obj");
	//mesh.ComputeBoundingBox();
	//model = cy::Matrix4f::MatrixTrans((mesh.GetBoundMax() + mesh.GetBoundMin()) / 2.0f);
	glewInit();	
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);	
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, mesh.NV() * sizeof(cy::Point3f), &mesh.V(0), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);	
	CompileShaders();

	glutMainLoop();
}

void Display() {
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(color.r, color.g, color.b, color.a);
	mvp = projection * cameraTranslation * cameraRotation * model;
	program.Bind();
	program.SetUniformMatrix4("mvp", mvp.data);
	glBindVertexArray(VAO);
	glDrawArrays(GL_POINTS, 0, mesh.NV());
	glutSwapBuffers();
}

void Idle() {
	glutPostRedisplay();
}

void GetKeyboardInput(unsigned char key, int xmouse, int ymouse) {
	if (key == 27) {
		glutLeaveMainLoop();
	}
}

void GetKeySpecial(int key, int xmouse, int ymouse) {
	if (key == GLUT_KEY_F6) {
		CompileShaders();
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
		yRot += xDiff;
		xRot += yDiff;
		cameraRotation = cameraRotation.MatrixRotationY(yRot) * cameraRotation.MatrixRotationX(xRot);
	}
	else {
		float diff = static_cast<float>(y) - lastRightMousePos;
		lastRightMousePos = static_cast<float>(y);
		cameraTranslation.AddTrans(-cy::Point3f(0.0f, 0.0f, diff));
	}
}

void CompileShaders() {
	vertexShader.CompileFile("Assets/Shaders/Vertex/mesh.glsl", GL_VERTEX_SHADER);
	fragmentShader.CompileFile("Assets/Shaders/Fragment/mesh.glsl", GL_FRAGMENT_SHADER);
	program.Build(&vertexShader, &fragmentShader);
}