///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.
	***/

	bool bReturn = false;
	//Creates GL texture for floor
	bReturn = CreateGLTexture(
		"../../Utilities/textures/Ancient Flooring.JPG",
		"Floor");
	//Creates GL texture for wall
	bReturn = CreateGLTexture(
		"../../Utilities/textures/backdrop.jpg",
		"Wall");
	//Creates GL texture for table
	bReturn = CreateGLTexture(
		"../../Utilities/textures/Wood_Floor.jpg",
		"Table");
	//Creates GL texture for keyboard
	bReturn = CreateGLTexture(
		"../../Utilities/textures/keyboard.jpg",
		"keyboard");
	//Creates GL texture for light bulb
	bReturn = CreateGLTexture(
		"../../Utilities/textures/Light.jpg",
		"light");
	//Creates GL texture for top of can
	bReturn = CreateGLTexture(
		"../../Utilities/textures/CanTop.jpg",
		"cantop");
	//Creates GL texture for side of can
	bReturn = CreateGLTexture(
		"../../Utilities/textures/CanSide.jpg",
		"canside");
	//Creates GL texture for bookcover
	bReturn = CreateGLTexture(
		"../../Utilities/textures/book.jpg",
		"bookcover");
	//Creates GL texture for book pages
	bReturn = CreateGLTexture(
		"../../Utilities/textures/paperpages.jpg",
		"bookpages");
	

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}
void SceneManager::DefineObjectMaterials()
{
	//shaders for wood material
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 10.0;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	//shaders for glass material
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	//shaders for wall
	OBJECT_MATERIAL wallMaterial;
	wallMaterial.ambientColor = glm::vec3(0.6f, 0.6f, 0.6f);
	wallMaterial.ambientStrength = 0.6f;
	wallMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.1f);
	wallMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	wallMaterial.shininess = 0.0;
	wallMaterial.tag = "wall";

	m_objectMaterials.push_back(wallMaterial);

	//shaders for floor
	OBJECT_MATERIAL floorMaterial;
	floorMaterial.ambientColor = glm::vec3(0.6f, 0.6f, 0.6f);
	floorMaterial.ambientStrength = 0.6f;
	floorMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.1f);
	floorMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	floorMaterial.shininess = 0.0;
	floorMaterial.tag = "floor";

	m_objectMaterials.push_back(floorMaterial);

	//shaders for metal material
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	metalMaterial.ambientStrength = 0.3f;
	metalMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	metalMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	metalMaterial.shininess = 22.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);

	//shaders for book material
	OBJECT_MATERIAL bookMaterial;
	bookMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	bookMaterial.ambientStrength = 0.2f;
	bookMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	bookMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	bookMaterial.shininess = 10.0;
	bookMaterial.tag = "book";

	m_objectMaterials.push_back(bookMaterial);

}


void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	//ceiling light source
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 42.0f, 0.0f);
	
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.2f);

	//lamp light source
	m_pShaderManager->setVec3Value("lightSources[1].position", -5.85f, 20.0f, -12.95f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.2f);



}


/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	//loads textures for 3D scene
	LoadSceneTextures();

	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();

	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadPyramid3Mesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh(.1);
	m_basicMeshes->LoadTaperedCylinderMesh();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	RenderRoom();
	RenderCeilingLight();
	RenderTable();
	RenderLaptop();
	RenderLamp();
	RenderCan();
	RenderBooks();
}
void SceneManager::RenderRoom() {
	//Floor

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(21.0f, 1.0f, 21.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets texture
	SetShaderTexture("Floor");
	SetTextureUVScale(1.0, 1.0);
	//floor material
	SetShaderMaterial("floor");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	//Ceiling

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(21.0f, 1.0f, 21.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 42.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets texture
	SetShaderTexture("Floor");
	SetTextureUVScale(1.0, 1.0);
	//floor material
	SetShaderMaterial("floor");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	//Far Wall

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(21.0f, 1.0f, 21.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 21.0f, -21.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets texture
	SetShaderTexture("Wall");
	SetTextureUVScale(1.0, 1.0);
	//wall material
	SetShaderMaterial("wall");


	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	//Near Wall

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(21.0f, 1.0f, 21.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 21.0f, 21.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets texture
	SetShaderTexture("Wall");
	SetTextureUVScale(1.0, 1.0);
	//wall material
	SetShaderMaterial("wall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	//Left Wall

	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(21.0f, 1.0f, 21.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-21.0f, 21.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets texture
	SetShaderTexture("Wall");
	SetTextureUVScale(1.0, 1.0);
	//wall material
	SetShaderMaterial("wall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	//right Wall

	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(21.0f, 1.0f, 21.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(21.0f, 21.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//wall texture
	SetShaderTexture("Wall");
	SetTextureUVScale(1.0, 1.0);

	//wall material
	SetShaderMaterial("wall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

}

void SceneManager::RenderCeilingLight() {

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 3.0f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 42.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//light source
	SetShaderColor(1, 1, 1, 1);
	//material glass
	SetShaderMaterial("glass");


	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
}

void SceneManager::RenderTable() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//**************************************
	//Table Top

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(28.0f, 1.0f, 14.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 14.0f, -14.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets texture
	SetShaderTexture("Table");
	SetTextureUVScale(1.0, 1.0);
	//material wood
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//Right Leg

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 28.0f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.0f, 0.0f, -14.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets texture
	SetShaderTexture("Table");
	SetTextureUVScale(1.0, 1.0);
	//material wood
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	//Back Left Leg

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 28.0f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.0f, 0.0f, -14.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets texture
	SetShaderTexture("Table");
	SetTextureUVScale(1.0, 1.0);
	//material wood
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}


void SceneManager::RenderLaptop() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//**************************************
	//Laptop Bottom
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.0f, 0.5f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 14.5f, -14.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	//keyboard texture
	SetShaderTexture("keyboard");
	SetTextureUVScale(1, 1);
	//sets material to metal
	SetShaderMaterial("metal");
	//draws this side to have keyboard
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::top);

	//sets color to grey
	SetShaderColor(0.627f, 0.627f, 0.627f,1);
	//shader material metal
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	

	//Laptop Back

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.0f, 0.25f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 17.5f, -17.121f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//screen color is black
	SetShaderColor(0, 0, 0, 1);
	//shader matierial is metal
	SetShaderMaterial("metal");
	//draws just a particular side of the mesh with the color black for the screen
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::top);

	//set color to grey
	SetShaderColor(0.627f, 0.627f, 0.627f,1);

	//set shader material to metal
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

}

void SceneManager::RenderLamp() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/


	//Base
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 1.0f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 15.0f, -15.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets color
	SetShaderColor(0.031, 0.031, 0.031, 1);

	//set shader material to metal
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();


	//cylinder rod

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.33f, 8.0f, 0.33f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 14.5f, -15.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets color
	SetShaderColor(0.031, 0.031, 0.031, 1);

	//set shader material to metal
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//rod to bulb connection

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.66f, 0.50f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 22.5f, -15.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	
	//sets color
	SetShaderColor(0.031, 0.031, 0.031, 1);

	//set shader material to metal
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//rod to bulb connection

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 1.5f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 135.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.95f, 23.0f, -14.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets color
	SetShaderColor(0.031, 0.031, 0.031, 1);

	//set shader material to metal
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfTorusMesh();

	//rod to bulb connection

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.66f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.85f, 22.5f, -12.95f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets color
	SetShaderColor(0.031, 0.031, 0.031, 1);

	//set shader material to metal
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//bulb connection

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 3.0f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.85f, 21.5f, -12.95f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets color
	SetShaderColor(0.031, 0.031, 0.031, 1);

	//set shader material to metal
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();

	//bulb 

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 0.1f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.85f, 20.0f, -12.95f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//lightbulb texture
	SetShaderTexture("light");
	SetTextureUVScale(1, 1);
	//sets material to metal
	SetShaderMaterial("glass");
	//draws this side to have keyboard
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::bottom);

	//sets color
	SetShaderColor(0.031, 0.031, 0.031, 1);
	//shader material metal
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::RenderCan() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	//can

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.75f, 3.0f, 0.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 14.51f, -14.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//can texture
	SetShaderTexture("canside");
	SetTextureUVScale(1, 1);
	//sets material to metal
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false,false,true);

	//can texture
	SetShaderTexture("cantop");
	SetTextureUVScale(1, 1);
	//sets material to metal
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(true, false, false);

	//bottom torus

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.70f, 0.70f, 0.33f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 14.5f, -14.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets color to grey
	SetShaderColor(0.627f, 0.627f, 0.627f, 1);
	//shader material metal
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	//top torus

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.70f, 0.70f, 0.33f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 17.5f, -14.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//sets color to grey
	SetShaderColor(0.627f, 0.627f, 0.627f, 1);
	//shader material metal
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
}
void SceneManager::RenderBooks() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	//book

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 1.0f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(9.0f, 15.0f, -16.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//draws just a particular side of the mesh with the book cover texture
	SetShaderTexture("bookcover");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("book");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::left);

	//draws just a particular side of the mesh with the book cover texture
	SetShaderTexture("bookcover");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("book");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::top);

	// draw the mesh with book pages texture
	SetShaderTexture("bookpages");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("book");
	m_basicMeshes->DrawBoxMesh();
	
}
