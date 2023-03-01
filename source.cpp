// ----------------------------------------------------------------------------
// Simple sample to prove that Assimp is easy to use with OpenGL.
// It takes a file name as command line parameter, loads it using standard
// settings and displays it.
//
// If you intend to _use_ this code sample in your app, do yourself a favour 
// and replace immediate mode calls with VBOs ...
//
// The vc8 solution links against assimp-release-dll_win32 - be sure to
// have this configuration built.
// ----------------------------------------------------------------------------
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>

// assimp include files. These three are usually needed.
#include "assimp.h"
#include "aiPostProcess.h"
#include "aiScene.h"

#include "GL/glut.h"
#include <IL/il.h>

#include <../irrklang/include/irrKlang.h> //audio library
using namespace irrklang;
ISoundEngine* SoundEngine = createIrrKlangDevice();
ISoundEngine* Soundtrack = createIrrKlangDevice();
bool music_flag = false;

//to map image filenames to textureIds
#include <string.h>
#include <map>
#define N 5


// currently this is hardcoded
//static const std::string basepath = "./models/textures/"; //obj..
static const std::string basepath = "./models/"; //for blend file

// the global Assimp scene object
const struct aiScene* scene = NULL;
GLuint scene_list = 0;
struct aiVector3D scene_min, scene_max, scene_center;

float x_coord_background = 0.0;
float x_coord_background2 = 13.1;
float y_coord_char = 0.0;
float x_coord_object[N] = { -16, -13, -10, -7, -4 };
float y_coord_object[N] = {};
float z_coord_object[N] = { 0 };
float x_coord_fireball = 0;
float y_coord_fireball = 0;
float z_coord_fireball = 0;
float up = 2.7, up2 = 5.4, down = -2.7, down2 = -5.4, center = 0;
float x_hit_box = 0.8, y_hit_box = 1;
float soglia = -16; //threshold objects separation
bool collision = false;
bool collision_fire = false;
bool fireball = false; //true if i clicked
bool first_fire = true;
bool fireball_timer = true; //can throw fireball
int index[N];
bool index_bool[N] = { false };
bool menu = true;
bool game = false;
bool guide = false;
bool game_over = false;
bool mode = false;
bool hard = false;
float mode_speed = 1.3;

int rand_back_1 = 0;
int rand_back_2 = 0;

float score = 0;
int highscore = 0;
int hp = 100;
int dagger_damage = 25;
int axe_damage = 50;
int coin_value = 10;
int chest_value = 100;
int red_potion_healing = 50;
bool shield = false;
bool chaos = false;
int fly_position = 0;
int fly_increment = 0;
float x_menu_option_sx = 0.23; 
float x_menu_option_dx = 0.76;

//objects index
static const int char_id = 13;
static const int dragon_top_id = 12;
static const int dragon_mid_id = 14;
static const int dragon_down_id = 15;
static const int coin_id = 1;
static const int axe_id = 10;
static const int chaos_id = 5;
static const int chest_id = 9;
static const int dagger_id = 8;
static const int potion_id = 6;
static const int shield_id = 7;
static const int menu_id = 4;
static const int guide_id = 3;
static const int gameover_id = 2;
static const int background_id = 11;
static const int background_door_id = 0;
static const int background_win_id = 17;
static const int mode_id = 16;
static const int max_hp = 21; 
static const int halfmax_hp = 22; 
static const int half_hp = 23;
static const int min_hp = 24;
static const int chaos_icon = 19;
static const int shield_icon = 20;
static const int fireball_id = 25;
static const int fire_icon = 26;
static const int fire_icon_off = 27;

// images / texture
std::map<std::string, GLuint*> textureIdMap;	// map image filenames to textureIds
GLuint* textureIds;							// pointer to texture Array

GLfloat LightAmbient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat LightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat LightPosition[] = { 0.0f, 0.0f, 15.0f, 1.0f };

#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)
#define TRUE                1
#define FALSE               0



// ----------------------------------------------------------------------------
void reshape(int width, int height)
{
	const double aspectRatio = (float)width / height, fieldOfView = 45.0;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fieldOfView, aspectRatio,
		1.0, 1000.0);  // Znear and Zfar 
	glViewport(0, 0, width, height);
}

// ----------------------------------------------------------------------------
void get_bounding_box_for_node(const struct aiNode* nd,
	struct aiVector3D* min,
	struct aiVector3D* max,
	struct aiMatrix4x4* trafo
) {
	struct aiMatrix4x4 prev;
	unsigned int n = 0, t;

	prev = *trafo;
	aiMultiplyMatrix4(trafo, &nd->mTransformation);

	for (; n < nd->mNumMeshes; ++n) {
		const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
		for (t = 0; t < mesh->mNumVertices; ++t) {

			struct aiVector3D tmp = mesh->mVertices[t];
			aiTransformVecByMatrix4(&tmp, trafo);

			min->x = aisgl_min(min->x, tmp.x);
			min->y = aisgl_min(min->y, tmp.y);
			min->z = aisgl_min(min->z, tmp.z);

			max->x = aisgl_max(max->x, tmp.x);
			max->y = aisgl_max(max->y, tmp.y);
			max->z = aisgl_max(max->z, tmp.z);
		}
	}

	for (n = 0; n < nd->mNumChildren; ++n) {
		get_bounding_box_for_node(nd->mChildren[n], min, max, trafo);
	}
	*trafo = prev;
}

// ----------------------------------------------------------------------------

void get_bounding_box(struct aiVector3D* min, struct aiVector3D* max)
{
	struct aiMatrix4x4 trafo;
	aiIdentityMatrix4(&trafo);

	min->x = min->y = min->z = 1e10f;
	max->x = max->y = max->z = -1e10f;
	get_bounding_box_for_node(scene->mRootNode, min, max, &trafo);
}

// ----------------------------------------------------------------------------

void color4_to_float4(const struct aiColor4D* c, float f[4])
{
	f[0] = c->r;
	f[1] = c->g;
	f[2] = c->b;
	f[3] = c->a;
}

// ----------------------------------------------------------------------------

void set_float4(float f[4], float a, float b, float c, float d)
{
	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;
}

// ----------------------------------------------------------------------------
void apply_material(const struct aiMaterial* mtl)
{
	float c[4];

	GLenum fill_mode;
	int ret1, ret2;
	struct aiColor4D diffuse;
	struct aiColor4D specular;
	struct aiColor4D ambient;
	struct aiColor4D emission;
	float shininess, strength;
	int two_sided;
	int wireframe;
	int max;

	int texIndex = 0;
	aiString texPath;	//contains filename of texture
	if (AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, texIndex, &texPath))
	{
		//bind texture
		unsigned int texId = *textureIdMap[texPath.data];
		glBindTexture(GL_TEXTURE_2D, texId);
	}

	set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
	if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
		color4_to_float4(&diffuse, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);

	set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
	if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
		color4_to_float4(&specular, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);

	set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
	if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
		color4_to_float4(&ambient, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);

	set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
	if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
		color4_to_float4(&emission, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);

	max = 1;
	ret1 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, (unsigned int*)&max);
	max = 1;
	ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, (unsigned int*)&max);
	if ((ret1 == AI_SUCCESS) && (ret2 == AI_SUCCESS))
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
	else {
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
		set_float4(c, 0.0f, 0.0f, 0.0f, 0.0f);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
	}

	max = 1;
	if (AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe, (unsigned int*)&max))
		fill_mode = wireframe ? GL_LINE : GL_FILL;
	else
		fill_mode = GL_FILL;
	glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

	max = 1;
	if ((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, (unsigned int*)&max)) && two_sided)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
}

// ----------------------------------------------------------------------------

// Can't send color down as a pointer to aiColor4D because AI colors are ABGR.
void Color4f(const struct aiColor4D* color)
{
	glColor4f(color->r, color->g, color->b, color->a);
}

// ----------------------------------------------------------------------------

void recursive_render(const struct aiScene* sc, const struct aiNode* nd, float scale)
{
	unsigned int i;
	unsigned int n = 0, t;
	struct aiMatrix4x4 m = nd->mTransformation;

	printf("Node name: %s\n", nd->mName.data);

	//m.Scaling(aiVector3D(scale, scale, scale), m);

	// update transform
	m.Transpose();
	glPushMatrix();
	glMultMatrixf((float*)&m);

	// draw all meshes assigned to this node
	for (; n < nd->mNumMeshes; ++n)
	{
		const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];

		///
		printf("Drawing MESH with this name: %s\n", mesh->mName.data);

		apply_material(sc->mMaterials[mesh->mMaterialIndex]);


		if (mesh->HasTextureCoords(0))
			glEnable(GL_TEXTURE_2D);
		else
			glDisable(GL_TEXTURE_2D);
		if (mesh->mNormals == NULL)
		{
			glDisable(GL_LIGHTING);
		}
		else
		{
			glEnable(GL_LIGHTING);
		}

		if (mesh->mColors[0] != NULL)
		{
			glEnable(GL_COLOR_MATERIAL);
		}
		else
		{
			glDisable(GL_COLOR_MATERIAL);
		}



		for (t = 0; t < mesh->mNumFaces; ++t) {
			const struct aiFace* face = &mesh->mFaces[t];
			GLenum face_mode;

			switch (face->mNumIndices)
			{
			case 1: face_mode = GL_POINTS; break;
			case 2: face_mode = GL_LINES; break;
			case 3: face_mode = GL_TRIANGLES; break;
			default: face_mode = GL_POLYGON; break;
			}

			glBegin(face_mode);

			for (i = 0; i < face->mNumIndices; i++)		// go through all vertices in face
			{
				int vertexIndex = face->mIndices[i];	// get group index for current index
				if (mesh->mColors[0] != NULL)
					Color4f(&mesh->mColors[0][vertexIndex]);
				if (mesh->mNormals != NULL)

					if (mesh->HasTextureCoords(0))		//HasTextureCoords(texture_coordinates_set)
					{

						glTexCoord2f(mesh->mTextureCoords[0][vertexIndex].x, 1 - mesh->mTextureCoords[0][vertexIndex].y); //mTextureCoords[channel][vertex]
					}

				glNormal3fv(&mesh->mNormals[vertexIndex].x);
				glVertex3fv(&mesh->mVertices[vertexIndex].x);
			}

			glEnd();

		}

	}



	// draw all children
	for (n = 0; n < nd->mNumChildren; ++n)
	{
		recursive_render(sc, nd->mChildren[n], scale);
	}

	glPopMatrix();
}

void output(float x, float y, float z, std::string str)
{
	int len, i;

	//disable light to set color using glColor
	glDisable(GL_LIGHTING);

	glColor3f(1, 0, 0); //set text color
	glRasterPos3f(x, y, z);
	len = str.length();
	for (i = 0; i < len; i++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, str[i]);
	}
	glEnable(GL_LIGHTING);

}

//-----------------------------------------------------------------------------

void write_highscore(int score){
	std::string top = "0";

	int i=0;
	std::ifstream rf("models\\score.txt", std::ios::in); //open a file to perform read operation using file object
	if (rf.is_open()) { //checking whether the file is open
		rf >> top;
		rf.close(); //close the file object.
	}

	std::ofstream fw("models\\score.txt", std::ofstream::out);
	
	if (score > std::stoi(top)) {
		top = std::to_string(score);
	}

	highscore = std::stoi(top);

	if (fw.is_open()){
		fw << top;
		fw.close();
	}
	else std::cout << "Problem with opening file";
}

// ----------------------------------------------------------------------------
void do_motion(void) //moving background
{

	x_coord_background -= 0.03*mode_speed;
	x_coord_background2 -= 0.03*mode_speed;

	if (x_coord_background < -13.1) { //se esce dallo schermo allora riposiziono
		x_coord_background = 13.1;
		rand_back_1 = rand() % 4; //randomizzo il prossimo background
	}
	if (x_coord_background2 < -13.1) {
		x_coord_background2 = 13.1;
		rand_back_2 = rand() % 4;
	}

}

// ----------------------------------------------------------------------------

void drawTextNum(char num[], int numdigits, float xpos, float ypos, float zpos) //counting the score
{
	int len;
	int k;
	k = 0;
	len = numdigits - strlen(num);
	
	//disable light to set static color
	glDisable(GL_LIGHTING);

	glRasterPos3f(xpos, ypos, zpos);
	for (int i = 0; i <= numdigits - 1; i++)
	{
		if (i < len)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, '0');
		else
		{
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, num[k]);
			k++;
		}

	}
	glEnable(GL_LIGHTING);
}

// ----------------------------------------------------------------------------
int generate_index() { //genera oggetti random
	int x;
	int index;
	int time = glutGet(GLUT_ELAPSED_TIME);
	srand(time);
	x = rand() % 100;
	
	if (!hard) {
		if (x >= 0 && x < 25)
			index = coin_id;
		else if (x >= 25 && x < 50)
			index = dagger_id;
		else if (x >= 50 && x < 70)
			index = axe_id;
		else if (x >= 70 && x < 80)
			index = shield_id;
		else if (x >= 80 && x < 90)
			index = potion_id;
		else if (x >= 90 && x < 96)
			index = chaos_id;
		else if (x >= 96 && x < 100)
			index = chest_id;
	}
	else { //changing random range --> grow probab malus - less bonus (potion and shield)
		if (x >= 0 && x < 25)
			index = coin_id; //coin
		else if (x >= 25 && x < 55)
			index = dagger_id; //dagger
		else if (x >= 55 && x < 80)
			index = axe_id;	//axe
		else if (x >= 80 && x < 85)
			index = shield_id;	//shield
		else if (x >= 85 && x < 90)
			index = potion_id;	//red potion
		else if (x >= 90 && x < 96)
			index = chaos_id;	//chaos
		else if (x >= 96 && x < 100)
			index = chest_id;	//chest
	}

	return index;
}

void end_chaos(int value) { //after timer func
	chaos = false;
}

void active_fire(int value) { //after timer func
	fireball_timer = true;
}

void draw_menu() {

	glPushMatrix();
	glTranslatef(0, 0, 0);
	recursive_render(scene, scene->mRootNode->mChildren[menu_id], 1.0);
	glPopMatrix();
}

void draw_mode() {

	glPushMatrix();
	glTranslatef(0, 0, 0);
	recursive_render(scene, scene->mRootNode->mChildren[mode_id], 1.0);
	glPopMatrix();
}

void draw_guide() {

	glPushMatrix();
	glTranslatef(0, 0, 0);
	recursive_render(scene, scene->mRootNode->mChildren[guide_id], 1.0);
	glPopMatrix();
}

void draw_gameover() {

	glPushMatrix();
	glTranslatef(0, 0, 0);
	recursive_render(scene, scene->mRootNode->mChildren[gameover_id], 1.0);
	glPopMatrix();
}



void draw_background() {


	//draw_background1
	switch (rand_back_1)
	{
	case 0:	
		glPushMatrix();
		glTranslatef(x_coord_background, 0, 0);
		recursive_render(scene, scene->mRootNode->mChildren[background_id], 1.0);
		glPopMatrix();
		break;
	case 1:
		glPushMatrix();
		glTranslatef(x_coord_background, 0, -2);
		recursive_render(scene, scene->mRootNode->mChildren[background_door_id], 1.0);
		glPopMatrix();
		break;
	case 2:
		glPushMatrix();
		glTranslatef(x_coord_background, 0, -2);
		recursive_render(scene, scene->mRootNode->mChildren[background_win_id], 1.0);
		glPopMatrix();
		break;
	case 3:
		glPushMatrix();
		glTranslatef(x_coord_background, 0, 0);
		recursive_render(scene, scene->mRootNode->mChildren[background_id], 1.0);
		glPopMatrix();
		break;
	default:
		break;
	}


	//draw_background2
	switch (rand_back_2)
	{
	case 0:
		glPushMatrix();
		glTranslatef(x_coord_background2, 0, 0);
		recursive_render(scene, scene->mRootNode->mChildren[background_id], 1.0);
		glPopMatrix();
		break;
	case 1:
		glPushMatrix();
		glTranslatef(x_coord_background2, 0, -2);
		recursive_render(scene, scene->mRootNode->mChildren[background_door_id], 1.0);
		glPopMatrix();
		break;
	case 2:
		glPushMatrix();
		glTranslatef(x_coord_background2, 0, -2);
		recursive_render(scene, scene->mRootNode->mChildren[background_win_id], 1.0);
		glPopMatrix();
		break;
	case 3:
		glPushMatrix();
		glTranslatef(x_coord_background2, 0, 0);
		recursive_render(scene, scene->mRootNode->mChildren[background_id], 1.0);
		glPopMatrix();
		break;
	default:
		break;
	}

}

void draw_character() {

	glPushMatrix();
	glTranslatef(0, y_coord_char, 0);
	recursive_render(scene, scene->mRootNode->mChildren[char_id], 1.0);
	switch (fly_position) {
	case 0: //low wings
		recursive_render(scene, scene->mRootNode->mChildren[dragon_down_id], 1.0);
		break;
	case 1: //mid wings
		recursive_render(scene, scene->mRootNode->mChildren[dragon_mid_id], 1.0);
		break;
	case 2: //high wings
		recursive_render(scene, scene->mRootNode->mChildren[dragon_top_id], 1.0);
		break;
	case 3: //mid wings
		recursive_render(scene, scene->mRootNode->mChildren[dragon_mid_id], 1.0);
		break;
	default:
		break;
	}

	//to make a smooth motion
	fly_increment++;
	if (fly_increment <= 120) {
		if (!(fly_increment % 30))
			if (fly_position < 3)
				fly_position++;
			else {
				fly_position = 0;
				fly_increment = 0;
			}
	}
	glPopMatrix();
}

void draw_UI() {

	if (hp == 100) {
		glPushMatrix();
		glTranslatef(0, 0, 20);
		recursive_render(scene, scene->mRootNode->mChildren[max_hp], 1.0);
		glPopMatrix();
	}
	else if (hp == 75) {
		glPushMatrix();
		glTranslatef(0, 0, 20);
		recursive_render(scene, scene->mRootNode->mChildren[halfmax_hp], 1.0);
		glPopMatrix();
	}
	else if (hp == 50) {
		glPushMatrix();
		glTranslatef(0, 0, 20);
		recursive_render(scene, scene->mRootNode->mChildren[half_hp], 1.0);
		glPopMatrix();
	}
	else if (hp == 25) {
		glPushMatrix();
		glTranslatef(0, 0, 20);
		recursive_render(scene, scene->mRootNode->mChildren[min_hp], 1.0);
		glPopMatrix();
	}

	if (shield && chaos) {
		glPushMatrix();
		glTranslatef(8, 6, 20);
		recursive_render(scene, scene->mRootNode->mChildren[shield_icon], 1.0);
		glPopMatrix();
	}
	else if (shield) {
		glPushMatrix();
		glTranslatef(7.2, 6, 20);
		recursive_render(scene, scene->mRootNode->mChildren[shield_icon], 1.0);
		glPopMatrix();
	}

	if (chaos) {
		glPushMatrix();
		glTranslatef(7.4, 6, 20);
		recursive_render(scene, scene->mRootNode->mChildren[chaos_icon], 1.0);
		glPopMatrix();
	}

}

void draw_score() {
	int score_int;

	output(7.5, 6, 20, "Score: ");
	output(-3, 6, 20, "Highscore: ");
	score_int = (int)score;
	std::string s = std::to_string(score_int); 
	char* buffer = const_cast<char*>(s.c_str()); //to convert score in a char array passing to drawtextnum   
	drawTextNum(buffer, 6, 8.5, 6, 20);
	score += 0.02; //increase score while playing
	if (score > highscore) {
		highscore = score;
		std::string s = std::to_string(highscore);
		char* buffer = const_cast<char*>(s.c_str());
		drawTextNum(buffer, 6, -1.5, 6, 20);
	}
	else {
		std::string s = std::to_string(highscore);
		char* buffer = const_cast<char*>(s.c_str());
		drawTextNum(buffer, 6, -1.5, 6, 20);
	}
}

void draw_object() { //draw malus/bonus
	int i, position;

	for (i = 0; i < 5; i++) { //for every object (5 objects wave)
		x_coord_object[i] -= 0.03*mode_speed;
		if (index_bool[i] == false) { //if object is not generated yet
			index[i] = generate_index(); //generating random object
			index_bool[i] = true;
		}

		glPushMatrix(); 
		glTranslatef(x_coord_object[i], y_coord_object[i], z_coord_object[i]);
		recursive_render(scene, scene->mRootNode->mChildren[index[i]], 1.0);
		glPopMatrix();

		if (fireball) { //true if i clicked 
			if (x_coord_object[i]<x_coord_fireball) { //collision with fire
				if (abs(y_coord_fireball - y_coord_object[i]) < y_hit_box)
					if (z_coord_object[i] == 0)
						if(index[i]==dagger_id || index[i]==axe_id || index[i]==chaos_id) //deleting just malus
							collision_fire = true;
			}

			if (collision_fire == true) {
				z_coord_object[i] = -100; //object not visible anymore
				collision_fire = false;
			}
		}
		
		if (x_coord_object[i] > -x_hit_box && x_coord_object[i] < x_hit_box) { //collision whit character
			if (abs(y_coord_char - y_coord_object[i]) < y_hit_box)
				if (z_coord_object[i] == 0)
					collision = true;
		}


		if (collision == true) {
			z_coord_object[i] = -100; 
			switch (index[i]) {
			case axe_id: 
				if (shield == false) {
					SoundEngine->play2D("./models/audio/knife.wav", false);
					hp -= axe_damage;
					if (hp <= 0) {
						game_over = true;
						game = false;
					}
				}
				else {
					SoundEngine->play2D("./models/audio/shield.wav", false);
					shield = false;
				}
				break;
				case chaos_id: 
				if (shield == false) {
					SoundEngine->play2D("./models/audio/potion.wav", false);
					chaos = true;
					glutTimerFunc(15000, end_chaos, 1); //effect ends in 15s
				}
				else {
					SoundEngine->play2D("./models/audio/shield.wav", false);
					shield = false;
				}
				break;
			case chest_id: 
				SoundEngine->play2D("./models/audio/bonus.wav", false);
				score += chest_value;
				break;
			case coin_id:
				SoundEngine->play2D("./models/audio/coin.wav", false);
				score += coin_value;
				break;
			case dagger_id:
				if (shield == false) {
					SoundEngine->play2D("./models/audio/knife.wav", false);
					hp -= dagger_damage;
					if (hp <= 0) {
						game_over = true;
						game = false;
					}
				}
				else {
					SoundEngine->play2D("./models/audio/shield.wav", false);
					shield = false;
				}
				break;
			case potion_id: 
				SoundEngine->play2D("./models/audio/potion.wav", false);
				if (hp + red_potion_healing > 100) {
					hp = 100;
				}
				else {
					hp += red_potion_healing;
				}
				break;
			case shield_id: 
				SoundEngine->play2D("./models/audio/bonus.wav", false);
				shield = true;
				break;
			default:
				break;
			}
			collision = false;
		}

		if (x_coord_object[i] < soglia) { //exceeding window size
			x_coord_object[i] = 13.2;
			z_coord_object[i] = 0;
			index_bool[i] = false;
			position = rand() % 5;
			switch (position) { //random position (among the 5 possibilities)
			case 0:
				y_coord_object[i] = center;
				break;
			case 1:
				y_coord_object[i] = up;
				break;
			case 2:
				y_coord_object[i] = up2;
				break;
			case 3:
				y_coord_object[i] = down;
				break;
			case 4:
				y_coord_object[i] = down2;
				break;
			default:
				break;
			}

			if (i == 4) {  //to reduce gap between firt 5 objects wave and the next 
				soglia = -2;
			}
		}

	}

}

void draw_fireicon(int id) {
	glPushMatrix();
	glTranslatef(3.65, 6, 20);
	recursive_render(scene, scene->mRootNode->mChildren[id], 1.0);
	glPopMatrix();
}

void draw_fireball() {
	x_coord_fireball += 0.03*mode_speed; //speed

	if (first_fire) {
		y_coord_fireball = y_coord_char;
		first_fire = false;
	}

	glPushMatrix();
	glTranslatef(x_coord_fireball, y_coord_fireball, z_coord_fireball);
	recursive_render(scene, scene->mRootNode->mChildren[fireball_id], 1.0);
	glPopMatrix();

	if (x_coord_fireball > 13.1) {
		x_coord_fireball = 0;
		first_fire = true;
		fireball = false;
	}

}

// ----------------------------------------------------------------------------

void mouse(int button, int state, int x, int y)
{
	int width = glutGet(GLUT_WINDOW_WIDTH);
	int height = glutGet(GLUT_WINDOW_HEIGHT);
	//using current width and height to adapt mouse coordinates

	if (menu) {
		switch (button) {
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_UP) {
				if (x > x_menu_option_sx * width && x < x_menu_option_dx * width) {
					if (y > 0.21 * height && y < 0.33 * height) { //play
						SoundEngine->play2D("./models/audio/click.wav",false);
						mode = true;
						menu = false;
						glutPostRedisplay();
					}
					else if (y > 0.44 * height && y < 0.56 * height) { //guide
						SoundEngine->play2D("./models/audio/click.wav", false);
						guide = true;
						menu = false;
						glutPostRedisplay();
					}
					else if (y > 0.66 * height && y < 0.78 * height) { //exit
						exit(1);
					}
				}
			}
			break;
		default:
			break;

		}
	}
	else if (mode) {
		switch (button) {
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_UP) {
				if (x > x_menu_option_sx * width && x < x_menu_option_dx * width) {
					if (y > 0.28 * height && y < 0.4 * height) { //easy
						SoundEngine->play2D("./models/audio/click.wav", false);
						mode = false;
						game = true;
						glutPostRedisplay();
					}
					else if (y > 0.56 * height && y < 0.675 * height) { //hard
						SoundEngine->play2D("./models/audio/click.wav", false);
						mode = false;
						game = true;
						hard = true;
						mode_speed = 2;
						dagger_damage = 2 * dagger_damage;
						axe_damage = 2 * axe_damage;
						glutPostRedisplay();
					}
				}
			}
			break;
		case GLUT_RIGHT_BUTTON:
			if (state == GLUT_DOWN) {
				SoundEngine->play2D("./models/audio/click.wav", false);
				mode = false;
				menu = true;
				glutPostRedisplay();
			}
			break;
		default:
			break;

		}
	}
	else if (guide) {
		switch (button) {
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_UP) {
				if (x > 0 && y > 0) {
					SoundEngine->play2D("./models/audio/click.wav", false);
					menu = true;
					guide = false;
					glutPostRedisplay();
				}
			}
			break;
		default:
			break;

		}

	}
	else if (game_over) {
		switch (button) {
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_UP) {
				if (x > 0 && y > 0) {
					SoundEngine->play2D("./models/audio/click.wav", false);
					int tmp = -16;
					//RESET ALL VARIABLES
					x_coord_background = 0.0;
					x_coord_background2 = 13.1;
					y_coord_char = 0.0;
					soglia = -16;
					for (int i = 0; i < 5; i++) {
						index_bool[i] = false;
						x_coord_object[i] = tmp;
						y_coord_object[i] = 0;
						z_coord_object[i] = 0;
						tmp += 3;
					}
					mode_speed = 1.3;
					menu = true;
					game_over = false;
					hp = 100;
					chaos = false;
					score = 0;
					dagger_damage = 25;
					axe_damage = 50;
					glutPostRedisplay();
				}
			}
			break;
		default:
			break;

		}
	}
	else if (game) {
		switch (button) {
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_UP) {
				if (x > 0 && y > 0) {
					if (fireball_timer) {//if i can shoot (15sec cooldown)
						SoundEngine->play2D("./models/audio/fireball.flac", false);
						//z_coord_fireball = 0;
						fireball = true;
						fireball_timer = false;
						glutTimerFunc(15000, active_fire, 1); //after 15sec i call active_fire function
					}
				}
			}
			break;
		default:
			break;
		}

	}
}
// ----------------------------------------------------------------------------
void display(void)
{
	float tmp;
	int time = 0;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-0.5, 0.5, -0.5, 0.5, 1, 10);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.f, 0.f, 3.f, 0.f, 0.f, -5.f, 0.f, 1.f, 0.f);


	//scale the whole asset to fit into our view frustum 
	tmp = scene_max.x - scene_min.x;
	tmp = aisgl_max(scene_max.y - scene_min.y, tmp);
	tmp = aisgl_max(scene_max.z - scene_min.z, tmp);
	tmp = 1.f / tmp;
	glScalef(tmp, tmp, tmp);

	// center the model
	glTranslatef(-scene_center.x, -scene_center.y, -scene_center.z);

	if (!music_flag) {
		Soundtrack->play2D("./models/audio/soundtrack.wav", true);
		music_flag = true;
	}

	if (game) {
		write_highscore(score);
		draw_background();
		draw_character();
		if (fireball)
			draw_fireball();
		if (fireball_timer)
			draw_fireicon(fire_icon);
		else
			draw_fireicon(fire_icon_off);
		draw_object();
		draw_score();
		draw_UI();
		do_motion();
	}
	else if (menu) {
		draw_menu();
	}
	else if (mode) {
		draw_mode();
	}
	else if (guide) {
		draw_guide();
	}
	else if (game_over) {
		draw_gameover();
		write_highscore(score);
	}

	glutPostRedisplay();
	glutSwapBuffers();

}

void specialKeyListener(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_UP:
		if (chaos == false) {
			if (y_coord_char < up2) {
				y_coord_char += up;
			}
			break;
		}
		else {
			if (y_coord_char > down2) {
				y_coord_char += down;
			}
			break;
		}
	case GLUT_KEY_DOWN:

		if (chaos == false) {
			if (y_coord_char > down2) {
				y_coord_char += down;
			}
			break;
		}
		else {
			if (y_coord_char < up2) {
				y_coord_char += up;
			}
			break;
		}
	}
	glutPostRedisplay();
}

// ----------------------------------------------------------------------------

int loadasset(const char* path)
{
	// we are taking one of the postprocessing presets to avoid
	// writing 20 single postprocessing flags here.
	scene = aiImportFile(path, aiProcessPreset_TargetRealtime_Quality);

	if (scene) {
		get_bounding_box(&scene_min, &scene_max);
		scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
		scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
		scene_center.z = (scene_min.z + scene_max.z) / 2.0f;
		return 0;
	}
	return 1;
}

int LoadGLTextures(const aiScene* scene)
{
	ILboolean success;

	/* Before calling ilInit() version should be checked. */
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
	{
		ILint test = ilGetInteger(IL_VERSION_NUM);
		/// wrong DevIL version ///
		std::string err_msg = "Wrong DevIL version. Old devil.dll in system32/SysWow64?";
		char* cErr_msg = (char*)err_msg.c_str();

		return -1;
	}

	ilInit(); /* Initialization of DevIL */

	//if (scene->HasTextures()) abortGLInit("Support for meshes with embedded textures is not implemented");

	/* getTexture Filenames and Numb of Textures */
	for (unsigned int m = 0; m < scene->mNumMaterials; m++)
	{
		int texIndex = 0;
		aiReturn texFound = AI_SUCCESS;

		aiString path;	// filename

		while (texFound == AI_SUCCESS)
		{
			texFound = scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
			textureIdMap[path.data] = NULL; //fill map with textures, pointers still NULL yet
			texIndex++;
		}
	}

	int numTextures = textureIdMap.size();

	/* array with DevIL image IDs */
	ILuint* imageIds = NULL;
	imageIds = new ILuint[numTextures];

	/* generate DevIL Image IDs */
	ilGenImages(numTextures, imageIds); /* Generation of numTextures image names */

	/* create and fill array with GL texture ids */
	textureIds = new GLuint[numTextures];
	glGenTextures(numTextures, textureIds); /* Texture name generation */

	/* define texture path */
	//std::string texturepath = "../../../test/models/Obj/";

	/* get iterator */
	std::map<std::string, GLuint*>::iterator itr = textureIdMap.begin();

	for (int i = 0; i < numTextures; i++)
	{

		//save IL image ID
		std::string filename = (*itr).first;  // get filename
		(*itr).second = &textureIds[i];	  // save texture id for filename in map
		itr++;								  // next texture


		ilBindImage(imageIds[i]); /* Binding of DevIL image name */
		std::string fileloc = basepath + filename;	/* Loading of image */
		success = ilLoadImage((const wchar_t*)fileloc.c_str());

		fprintf(stdout, "Loading Image: %s\n", fileloc.data());

		if (success) /* If no error occured: */
		{
			success = ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE); /* Convert every colour component into
			unsigned byte. If your image contains alpha channel you can replace IL_RGB with IL_RGBA */
			if (!success)
			{
				/* Error occured */
				fprintf(stderr, "Couldn't convert image");
				return -1;
			}
			//glGenTextures(numTextures, &textureIds[i]); /* Texture name generation */
			glBindTexture(GL_TEXTURE_2D, textureIds[i]); /* Binding of texture name */
			//redefine standard texture values
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); /* We will use linear
			interpolation for magnification filter */
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); /* We will use linear
			interpolation for minifying filter */
			glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH),
				ilGetInteger(IL_IMAGE_HEIGHT), 0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE,
				ilGetData()); /* Texture specification */
		}
		else
		{
			/* Error occured */
			fprintf(stderr, "Couldn't load Image: %s\n", fileloc.data());
		}
	}
	ilDeleteImages(numTextures, imageIds); /* Because we have already copied image data into texture data
	we can release memory used by image. */

	//Cleanup
	delete[] imageIds;
	imageIds = NULL;

	//return success;
	return TRUE;
}

int InitGL()					 // All Setup For OpenGL goes here
{
	if (!LoadGLTextures(scene))
	{
		return FALSE;
	}

	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);		 // Enables Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);				// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);		// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);			// The Type Of Depth Test To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculation

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT1);    // Uses default lighting parameters
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glEnable(GL_NORMALIZE);


	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
	glEnable(GL_LIGHT1);

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);

	return TRUE;					// Initialization Went OK
}

// ----------------------------------------------------------------------------
int main(int argc, char** argv)
{
	struct aiLogStream stream;
	glutInitWindowSize(900, 600);
	glutInitWindowPosition(100, 100);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInit(&argc, argv);

	glutCreateWindow("Daeron's Dungeon");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutSpecialFunc(specialKeyListener);

	// get a handle to the predefined STDOUT log stream and attach
	// it to the logging system. It will be active for all further
	// calls to aiImportFile(Ex) and aiApplyPostProcessing.

	stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
	aiAttachLogStream(&stream);

	// ... exactly the same, but this stream will now write the
	// log file to assimp_log.txt
	stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE, "assimp_log.txt");
	aiAttachLogStream(&stream);

	// the model name can be specified on the command line. 
	if (argc >= 2)
		loadasset(argv[1]);
	else // otherwise the model is specified statically 
	{
		char* modelToLoad = "models\\DungeonGame.obj";
		fprintf(stdout, "loading: %s", modelToLoad);
		loadasset(modelToLoad);
	}


	if (!InitGL())
	{
		fprintf(stderr, "Initialization failed");
		return FALSE;
	}

	glutGet(GLUT_ELAPSED_TIME);
	glutMainLoop();

	// cleanup - calling 'aiReleaseImport' is important, as the library 
	// keeps internal resources until the scene is freed again. Not 
	// doing so can cause severe resource leaking.
	aiReleaseImport(scene);

	// We added a log stream to the library, it's our job to disable it
	// again. This will definitely release the last resources allocated
	// by Assimp.
	aiDetachAllLogStreams();

	return 0;
}
