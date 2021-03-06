#include <Windows.h>
#include <GL/glew.h> // glew must be included before the main gl libs
#include <GL/glut.h> // doing otherwise causes compiler shouting

#include <iostream>
#include <ctime>
#include <string>
#include <fstream>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/color4.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> //Makes passing matrices to shaders easier

//M_PI does not appear to be defined when I build the project in visual studios
#define M_PI        3.14159265358979323846264338327950288   /* pi */

//--Data types
//This object will define the attributes of a vertex(position, color, etc...)
struct Vertex
{
    GLfloat position[3];
	GLfloat normal[3];
    GLfloat color[3];
};
struct Light
{
	GLfloat position[3];
	GLfloat color[3];
};

//--Evil Global variables
//Just for this example!
//Please don't do this in your code!

int w = 640, h = 480;// Window size
GLuint program;// The GLSL program handle
GLuint vbo_geometry;// VBO handle for our geometry
Vertex *geometry=NULL;// Pointer to geometry
int vertexCount=0;// Vertex count of geometry
glm::vec4 DP = glm::vec4(0.2,0.5,0.4,1.0);
glm::vec4 SP = glm::vec4(0.5,0.6,0.9,1.0);
float shininess = 100.0;

//point light
Light pointLight;

//uniform locations
GLint loc_modelView;
GLint loc_projection;
GLint loc_plColor;
GLint loc_plPosition;
GLint loc_dp;
GLint loc_sp;
GLint loc_shininess;

//attribute locations
GLint loc_position;
GLint loc_color;
GLint loc_norm;

//transform matrices
glm::mat4 model;//obj->world each object should have its own model matrix
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 mv;//premultiplied modelview

//--GLUT Callbacks
void render();
void update();
void reshape(int n_w, int n_h);
void keyboard(unsigned char key, int x_pos, int y_pos);

//--Load Obj 
bool loadObj(const char *filename, Vertex* &obj, int &vertexCount);

//--Resource management
bool initialize();
void cleanUp();

//--Time Difference
float getDT();
std::clock_t t1, t2;

//--Shader Loader
std::string loadShader(char* filename);

//--Main
int main(int argc, char **argv)
{
    // Initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(w, h);

    // Name and create the Window
    glutCreateWindow("Point Light Example");

    // Now that the window is created the GL context is fully set up
    // Because of that we can now initialize GLEW to prepare work with shaders
    GLenum status = glewInit();
    if( status != GLEW_OK)
    {
        std::cerr << "[F] GLEW NOT INITIALIZED: ";
        std::cerr << glewGetErrorString(status) << std::endl;
        return -1;
    }

    // Set all of the callbacks to GLUT that we need
    glutDisplayFunc(render);// Called when its time to display
    glutReshapeFunc(reshape);// Called if the window is resized
    glutIdleFunc(update);// Called if there is nothing else to do
    glutKeyboardFunc(keyboard);// Called if there is keyboard input

    // Initialize all of our resources(shaders, geometry)
    bool init = initialize();
    if(init)
    {
        t1 = std::clock();
        glutMainLoop();
    }

    // Clean up after ourselves
    cleanUp();
	system("PAUSE");
    return 0;
}

//--Implementations
void render()
{
    //--Render the scene

    //clear the screen
    glClearColor(0.0, 0.0, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //premultiply the matrix for this example
    mv = view * model;

    //enable the shader program
    glUseProgram(program);

    //upload the matrix to the shader
	glUniform4fv(loc_dp,1,glm::value_ptr(DP));
	glUniform4fv(loc_dp,1,glm::value_ptr(SP));
	glUniform1f(loc_shininess,shininess);
    glUniformMatrix4fv(loc_modelView, 1, GL_FALSE, glm::value_ptr(mv));
    glUniformMatrix4fv(loc_projection, 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3fv(loc_plColor, 1, pointLight.color);
	glUniform3fv(loc_plPosition, 1, pointLight.position);
	
    //set up the Vertex Buffer Object so it can be drawn
    glEnableVertexAttribArray(loc_position);
    glEnableVertexAttribArray(loc_color);
    glEnableVertexAttribArray(loc_norm);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    //set pointers into the vbo for each of the attributes(position and color)
    glVertexAttribPointer( loc_position,//location of attribute
                           3,//number of elements
                           GL_FLOAT,//type
                           GL_FALSE,//normalized?
                           sizeof(Vertex),//stride
                           (void*)offsetof(Vertex,position));//offset

    glVertexAttribPointer( loc_color,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           sizeof(Vertex),
                           (void*)offsetof(Vertex,color));

    glVertexAttribPointer( loc_norm,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           sizeof(Vertex),
                           (void*)offsetof(Vertex,normal));

    glDrawArrays(GL_TRIANGLES, 0, vertexCount);//mode, starting index, count

    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_color);
    glDisableVertexAttribArray(loc_norm);
                           
    //swap the buffers
    glutSwapBuffers();
}

void update()
{
    //total time
    static float angle = 0.0;
    float dt = getDT();// if you have anything moving, use dt.

    angle += dt * 90.0; //move through 90 degrees a second
	//because the model is not upright and it is too big to easily fix with blender
    glm::mat4 orientModel = glm::rotate( glm::mat4(1.0f), 100.0f, glm::vec3(1.0f,0.0f,0.0f));
	//spin model
    glm::mat4 rotateModel = glm::rotate( glm::mat4(1.0f), angle, glm::vec3(0.0f,1.0f,0.0f));

	//set model matrix
	model = rotateModel*orientModel;

    // Update the state of the scene
    glutPostRedisplay();//call the display callback
}


void reshape(int n_w, int n_h)
{
    w = n_w;
    h = n_h;
    //Change the viewport to be correct
    glViewport( 0, 0, w, h);
    //Update the projection matrix as well
    //See the init function for an explaination
    projection = glm::perspective(45.0f, float(w)/float(h), 0.01f, 100.0f);

}

void keyboard(unsigned char key, int x_pos, int y_pos)
{
    // Handle keyboard input
    if(key == 27)//ESC
    {
        exit(0);
    }
}

bool loadObj(const char *filename, Vertex* &obj, int &vertexCount)
{
	Assimp::Importer importer;
	bool hasColor = false;

	if(obj)
	{
        std::cerr << "[F] loadObj function used incorrectly." << std::endl;
		return false;
	}

	//load the file and make sure all polygons are triangles
	const aiScene *scene = importer.ReadFile(filename,aiProcess_Triangulate | aiProcess_GenNormals);
	
	if(!scene)
		return false;

	//get vertices from assimp
	aiVector3D *vertices = (*scene->mMeshes)->mVertices;
	//get normals from assimp
	aiVector3D *vertexNormals = (*scene->mMeshes)->mNormals;
	//get color information from assimp if exists
	aiColor4t<float> *colors = NULL;
	if((*scene->mMeshes)->HasVertexColors(0))
	{
		hasColor = true;
		colors = (*scene->mMeshes)->mColors[0];
	}

	//get vertex count from assimp
	vertexCount = (*scene->mMeshes)->mNumVertices;

	//allocate memory for obj
	obj = new Vertex[vertexCount];
	
	//add vertex, normals, and UVs to mesh object
	for(int i=0;i<vertexCount;++i)
	{
		obj[i].position[0] = vertices[i].x;
		obj[i].position[1] = vertices[i].y;
		obj[i].position[2] = vertices[i].z;
		
		obj[i].normal[0] = vertexNormals[i].x;
		obj[i].normal[1] = vertexNormals[i].y;
		obj[i].normal[2] = vertexNormals[i].z;

		if(hasColor)
		{
			obj[i].color[0] = colors[i].r;
			obj[i].color[1] = colors[i].g;
			obj[i].color[2] = colors[i].b;
		}
		else
		{
			obj[i].color[0] = 0.0;
			obj[i].color[1] = 1.0;
			obj[i].color[2] = 1.0;
		}
	}

	return true;
}

bool initialize()
{
	// Set light values
	pointLight.position[0] = 3.0f;
	pointLight.position[1] = 0.0f;
	pointLight.position[2] = -3.0f;

	pointLight.color[0] = 1.0f;
	pointLight.color[1] = 1.0f;
	pointLight.color[2] = 1.0f;

    // Initialize geometry and shaders for this example

    //this defines a cube, this is why a model loader is nice
    //you can also do this with a draw elements and indices, try to get that working
	std::cout << "Obj file is loading this might take a moment. Please wait." << std::endl;
	if(!loadObj("dragon.obj", geometry, vertexCount))
	{
        std::cerr << "[F] The obj file did not load correctly." << std::endl;
		return false;
	}

    // Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(1, &vbo_geometry);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    glBufferData(GL_ARRAY_BUFFER, vertexCount*sizeof(Vertex), geometry, GL_STATIC_DRAW);

    //--Geometry done

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    //Shader Sources
    // Put these into files and write a loader in the future
    // Note the added uniform!
    std::string vs = loadShader("VertexShader.txt");

    std::string fs = loadShader("FragShader.txt");

    //compile the shaders
    GLint shader_status;
	
	const char* _vs = vs.c_str();
	const char* _fs = fs.c_str();

    // Vertex shader first
    glShaderSource(vertex_shader, 1, &_vs, NULL);
    glCompileShader(vertex_shader);
    //check the compile status
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] FAILED TO COMPILE VERTEX SHADER!" << std::endl;
        return false;
    }

    // Now the Fragment shader
    glShaderSource(fragment_shader, 1, &_fs, NULL);
    glCompileShader(fragment_shader);
    //check the compile status
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] FAILED TO COMPILE FRAGMENT SHADER!" << std::endl;
        return false;
    }

    //Now we link the 2 shader objects into a program
    //This program is what is run on the GPU
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    //check if everything linked ok
    glGetProgramiv(program, GL_LINK_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] THE SHADER PROGRAM FAILED TO LINK" << std::endl;
        return false;
    }

    //Now we set the locations of the attributes and uniforms
    //this allows us to access them easily while rendering
    loc_position = glGetAttribLocation(program,
                    const_cast<const char*>("v_position"));
    if(loc_position == -1)
    {
        std::cerr << "[F] POSITION NOT FOUND" << std::endl;
        return false;
    }

    loc_color = glGetAttribLocation(program,
                    const_cast<const char*>("v_color"));
    if(loc_color == -1)
    {
        std::cerr << "[F] V_COLOR NOT FOUND" << std::endl;
        return false;
    }

    loc_norm = glGetAttribLocation(program,
                    const_cast<const char*>("v_norm"));
    if(loc_norm == -1)
    {
        std::cerr << "[F] V_NORM NOT FOUND" << std::endl;
        return false;
    }

    loc_dp = glGetUniformLocation(program,
                    const_cast<const char*>("DP"));
    if(loc_dp == -1)
    {
        std::cerr << "[F] DP NOT FOUND" << std::endl;
        return false;
    }

    loc_sp = glGetUniformLocation(program,
                    const_cast<const char*>("SP"));
    if(loc_sp == -1)
    {
        std::cerr << "[F] SP NOT FOUND" << std::endl;
        return false;
    }

    loc_shininess = glGetUniformLocation(program,
                    const_cast<const char*>("shininess"));
    if(loc_shininess == -1)
    {
        std::cerr << "[F] SHININESS NOT FOUND" << std::endl;
        return false;
    }

    loc_modelView = glGetUniformLocation(program,
                    const_cast<const char*>("ModelView"));
    if(loc_modelView == -1)
    {
        std::cerr << "[F] MODELVIEW NOT FOUND" << std::endl;
        return false;
    }

    loc_projection = glGetUniformLocation(program,
                    const_cast<const char*>("Projection"));
    if(loc_projection == -1)
    {
        std::cerr << "[F] PROJECTION NOT FOUND" << std::endl;
        return false;
    }

    loc_plColor = glGetUniformLocation(program,
                    const_cast<const char*>("pointLight.color"));
    if(loc_plColor == -1)
    {
        std::cerr << "[F] POINTLIGHTCOLOR NOT FOUND" << std::endl;
        return false;
    }

    loc_plPosition = glGetUniformLocation(program,
                    const_cast<const char*>("pointLight.position"));
    if(loc_plPosition == -1)
    {
        std::cerr << "[F] POINTLIGHTPOSITION NOT FOUND" << std::endl;
        return false;
    }
    
    //--Init the view and projection matrices
    //  if you will be having a moving camera the view matrix will need to more dynamic
    //  ...Like you should update it before you render more dynamic 
    //  for this project having them static will be fine
    view = glm::lookAt( glm::vec3(0.0, 8.0, -16.0), //Eye Position
                        glm::vec3(0.0, 0.0, 0.0), //Focus point
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up

    projection = glm::perspective( 45.0f, //the FoV typically 90 degrees is good which is what this is set to
                                   float(w)/float(h), //Aspect Ratio, so Circles stay Circular
                                   0.01f, //Distance to the near plane, normally a small value like this
                                   100.0f); //Distance to the far plane, 

    //enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    //and its done
    return true;
}

void cleanUp()
{
    // Clean up, Clean up
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo_geometry);
}

//returns the time delta
float getDT()
{
    float ret;
    t2 = std::clock();
    ret = double(t2-t1)/double(CLOCKS_PER_SEC);
    t1 = std::clock();
    return ret;
}

std::string loadShader(char* filename)
{
	int len;

	//open file as ASCII
	std::ifstream file;
	file.open(filename, std::ios::in); 

	//check if opened
	if(!file)
	{
        std::cerr << "[F] FAILED TO OPEN FILE" << std::endl;
		return NULL;
	}

	char *ShaderSource=NULL;

	//get length of data
	file.seekg(0,std::ios::end);
	len = int(file.tellg());
	file.seekg(std::ios::beg);
    
	//check if file is empty
	if (len==0)
	{
        std::cerr << "[F] FILE IS EMPTY" << std::endl;
		return NULL;
	}
    
	//allocate memory
	ShaderSource = new char[len+1];
	if (ShaderSource == 0) // can't reserve memory
	{
        std::cerr << "[F] CANNOT RESERVE MEMORY" << std::endl;
		return NULL;
	}
   
	// len isn't always strlen cause some characters are stripped in ascii read...
	// it is important to 0-terminate the real length later, len is just max possible value... 
	ShaderSource[len] = 0; 

	unsigned int i=0;
	while (file.good())
	{
		// get character from file
		ShaderSource[i] = file.get(); 
		if (!file.eof())
			i++;
	}
    
	// 0-terminate it at the correct position
	ShaderSource[i] = 0;
    
	file.close();

	std::string shader = std::string(ShaderSource);

	return shader;
}