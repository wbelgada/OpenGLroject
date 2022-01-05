#include "headers/scene.h"

void Scene::registerVertexOnShader(unsigned vertexIndex) {
	mat4 model = vertices.at(vertexIndex).getMatrix();

	program.activateShader();

	glUniformMatrix4fv(program.getUniformLocation("model"), 1, GL_FALSE, value_ptr(model));
}

void Scene::registerLightOnShader(unsigned lightIndex) {
	mat4 model = lights.at(lightIndex).getVAO().getMatrix();

	lights.at(lightIndex).getShader().activateShader();

	glUniformMatrix4fv(lights.at(lightIndex).getShader().getUniformLocation("model"), 1, GL_FALSE, value_ptr(model));
}

void Scene::updateLightsUni() {
	activateShader();

	vector<GLfloat> pos;
	vector<GLfloat> colors;
	vector<GLfloat> types;
	for (int i = 0; i < lights.size(); i++) {
		pos.push_back(lights.at(i).getVAO().getTranslation().r);
		pos.push_back(lights.at(i).getVAO().getTranslation().g);
		pos.push_back(lights.at(i).getVAO().getTranslation().b);
	}

	for (int i = 0; i < lights.size(); i++) {
		colors.push_back(lights.at(i).getR());
		colors.push_back(lights.at(i).getG());
		colors.push_back(lights.at(i).getB());
	}

	for (int i = 0; i < lights.size(); i++) {
		types.push_back(float(lights.at(i).getType()));
	}

	glUniform1i(program.getUniformLocation("lightSize"), lights.size());
	glUniform1fv(program.getUniformLocation("lightPos"), pos.size(), pos.data());
	glUniform1fv(program.getUniformLocation("lightColor"), colors.size(), colors.data());
	glUniform1fv(program.getUniformLocation("lightType"), types.size(), types.data());
}

void Scene::resetLightsUni() {
	activateShader();

	vector<GLfloat> pos;
	vector<GLfloat> colors;
	vector<GLfloat> types;

	pos.push_back(1.0f);
	pos.push_back(1.0f);
	pos.push_back(1.0f);

	colors.push_back(1.0f);
	colors.push_back(1.0f);
	colors.push_back(1.0f);

	types.push_back(float(LightType::DIRECT));

	glUniform1i(program.getUniformLocation("lightSize"), lights.size());
	glUniform1fv(program.getUniformLocation("lightPos"), pos.size(), pos.data());
	glUniform1fv(program.getUniformLocation("lightColor"), colors.size(), colors.data());
	glUniform1fv(program.getUniformLocation("lightType"), types.size(), types.data());
}

void Scene::translateOnShader(Shader& shader, unsigned vertexIndex, float xd, float yd, float zd, bool light) {
	VAO* vertice;
	string uniform;

	if (light)
		vertice = &lights.at(vertexIndex).getVAO();
	else
		vertice = &vertices.at(vertexIndex);

	vertice->translate(xd, yd, zd);
	vec3 position = vertice->getTranslation();

	mat4 translation = mat4(1.0f);
	translation = translate(translation, position);

	shader.activateShader();

	glUniformMatrix4fv(shader.getUniformLocation("translation"), 1, GL_FALSE, value_ptr(translation));
}

void Scene::rotateOnShader(Shader& shader, unsigned vertexIndex, float wd, float xd, float yd, float zd) {
	VAO* vertice = &vertices.at(vertexIndex);

	vertice->rotate(wd, xd, yd, zd);
	quat position = vertice->getRotation();

	mat4 rotation = mat4(1.0f);
	rotation = mat4_cast(position);

	shader.activateShader();

	glUniformMatrix4fv(shader.getUniformLocation("rotation"), 1, GL_FALSE, value_ptr(rotation));
}

void Scene::scaleOnShader(Shader& shader, unsigned vertexIndex, float xd, float yd, float zd) {
	vertices.at(vertexIndex).scale(xd, yd, zd);
	vec3 position = vertices.at(vertexIndex).getScale();

	mat4 scaling = mat4(1.0f);
	scaling = scale(scaling, position);

	shader.activateShader();

	glUniformMatrix4fv(shader.getUniformLocation("scale"), 1, GL_FALSE, value_ptr(scaling));
}

void Scene::alphaOnShader(Shader& shader, float alpha) {
	shader.activateShader();
	glUniform1i(shader.getUniformLocation("vAlpha"), alpha);
}

void Scene::setDepthUniform(Shader& shader) {
	shader.activateShader();
	glUniform4fv(shader.getUniformLocation("depthColor"), 1, value_ptr(depthColor));
}

void Scene::notifyCameraPosition(Camera* camera) {
	activateShader();

	glUniform3f(program.getUniformLocation("camPos"),
		camera->getPosition().x, camera->getPosition().y, camera->getPosition().z);
}


// CONSTRUCTOR
Scene::Scene(const char* vFile, const char* fFile, unsigned width, unsigned height, vector<string> skyFaces)
	: program(Shader(vFile, fFile)), pp(PostProcess("shaders/framebuffer.vs", "shaders/framebuffer.fs", width, height)),
	  sb(SkyBox("shaders/sky.vs", "shaders/sky.fs", skyFaces)) {

	pp.activateShader();
	pp.setFrameTextureUni(0);

	vec2 dim = vec2(float(width), float(height));
	glUniform2fv(pp.getUniformLocation("resolution"), 1, value_ptr(dim));

	// Depth
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Culling
	glCullFace(GL_FRONT);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
}

void Scene::render(GLFWwindow* window, Camera* camera, unsigned width, unsigned height) {
	pp.activateShader();

	glUniform1i(pp.getUniformLocation("ppType"), pp.getPPType());

	pp.bindFBO();

	setGLColor(depthColor.r, depthColor.g, depthColor.b, depthColor.a);

	camera->defineInputs(window);
	camera->updateMatrix(0.1f, 100.0f);

	setCameraMatrix(camera);

	draw(camera, width, height);

	pp.unbindFBO();
	pp.draw();

	glfwSwapBuffers(window);						// Swap the back and front buffer
	glfwPollEvents();								// Takes care of all GLFW events
}

void Scene::activateShader() {
	program.activateShader();
}

void Scene::activateLightShader(unsigned index) {
	lights.at(index).getShader().activateShader();
}

void Scene::destroy() {
	for (VAO vertex : vertices)
		vertex.destroy();

	program.deleteShader();
}

void Scene::draw(Camera* camera, unsigned width, unsigned height) {
	activateShader();

	updateLightsUni();
	setDepthUniform(program);

	glEnable(GL_BLEND);
	for (int i = 0; i < vertices.size(); i++) {
		vertices.at(i).bind();

		mat4 matrix = vertices.at(i).getMatrix();
		quat rotation = vertices.at(i).getRotation();
		vec3 scaling = vertices.at(i).getScale();

		bool oriBound = vertices.at(i).hasOrientationBounds();
		bool posiBound = vertices.at(i).hasPositionBounds();

		// Vertex Position
		if (posiBound) {
			vector<DFloat> binds = vertices.at(i).getPositionBounds();
			translateVertex(i, binds.at(0).value(), binds.at(1).value(), binds.at(2).value());
		}
		else {
			vec3 translation = vertices.at(i).getTranslation();
			translateVertex(i, translation.r, translation.g, translation.b);
		}

		// Vertex Rotation-Orientation
		if (oriBound) {
			vector<DFloat> oriBinds = vertices.at(i).getOrientationBounds();

			float x = oriBinds.at(0).value();
			float y = oriBinds.at(1).value();
			float z = oriBinds.at(2).value();

			program.activateShader();

			// TODO : rotation bound 
			mat4 look = glm::lookAt(vec3(0.0f, 0.0f, 0.0f), (vec3(-0.0f, 0.0f, -0.0f) + vec3(x, y, z)), vec3(0.0f, 1.0f, 0.0f));
			glUniformMatrix4fv(program.getUniformLocation("rotation"), 1, GL_FALSE, value_ptr(look));
		}
		else {
			quat rotation = vertices.at(i).getRotation();
			rotateOnShader(program, i, rotation.w, rotation.x, rotation.y, rotation.z);
		}

		scaleVertex(i, scaling.r, scaling.g, scaling.b);
		//alphaOnShader(program, 1.0f);

		registerVertexOnShader(i);

		vertices.at(i).registerMeshTextures(program);
		vertices.at(i).bindMeshTextures();
		vertices.at(i).draw();
	}

	for (int i = 0; i < lights.size(); i++) {
		vec3 translation = lights.at(i).getVAO().getTranslation();

		lights.at(i).getShader().activateShader();
		lights.at(i).getVAO().bind();

		// Light orientation
		if (lights.at(i).hasOrientationBounds()) {
			vector<DFloat> oriBinds = lights.at(i).getOrientationBounds();

			vec3 orientation = vec3(oriBinds.at(0).value(), oriBinds.at(1).value(), oriBinds.at(2).value());
			glUniform3fv(program.getUniformLocation("lightOrientation"),
				1, value_ptr(orientation));
		}
		else {
			glUniform3fv(program.getUniformLocation("lightOrientation"),
				1, value_ptr(lights.at(i).getOrientation()));
		}

		// Light position
		if (lights.at(i).getVAO().hasPositionBounds()) {
			vector<DFloat> binds = lights.at(i).getVAO().getPositionBounds();
			translateOnShader(lights.at(i).getShader(), i, binds.at(0).value(), binds.at(1).value(), binds.at(2).value(), true);
		}
		else
			translateOnShader(lights.at(i).getShader(), i, translation.r, translation.g, translation.b, true);
		
		registerLightOnShader(i);

		setDepthUniform(lights.at(i).getShader());

		lights.at(i).getVAO().draw();
	}
	glDisable(GL_BLEND);

	// SkyBox
	resetLightsUni();
	sb.draw(camera, width, height);
}

void Scene::setGLColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	// "Adds" a color to the back buffer
	glClearColor(red, green, blue, alpha);
	// Clears what's in the front buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Scene::setBackgroundColor(GLFWwindow* window, unsigned int width, unsigned int height, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	depthColor = vec4(red, green, blue, alpha);
	setGLColor(red, green, blue, alpha);
	glfwSwapBuffers(window);
}

void Scene::setPPType(PPType type) {
	pp.setPPType(type);
}

unsigned Scene::addMesh(Mesh obj, float posX, float posY, float posZ, float alpha) {
	activateShader();

	VAO vao(true);
	obj.bind();

	vao.addMesh(obj);

	vao.unbind();
	obj.unbind();

	vao.setAlpha(alpha);
	vertices.push_back(vao);

	unsigned index = vertices.size() - 1;
	translateVertex(index, posX, posY, posZ);

	return index;
}

vector<Texture> Scene::retrieveMeshTextures(const aiScene* pScene, aiMesh* aiMesh, const char* path) {
	vector<Texture> textures;

	const aiMaterial* material = pScene->mMaterials[aiMesh->mMaterialIndex];

	aiString aiPath;

	string fileStr = string(path);
	string fileDirectory = fileStr.substr(0, fileStr.find_last_of('/') + 1);

	if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &aiPath, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
			textures.push_back(Texture((fileDirectory + aiPath.data).c_str(), "tex0", 0));
	}

	if (material->GetTextureCount(aiTextureType_SPECULAR) > 0) {
		if (material->GetTexture(aiTextureType_SPECULAR, 0, &aiPath, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
			textures.push_back(Texture((fileDirectory + aiPath.data).c_str(), "tex1", 1));
	}

	return textures;
}

Mesh Scene::retrieveMesh(const aiScene* pScene, aiMesh* aiMesh, const char* path) {
	vector<GLfloat> vertices;
	vector<GLuint> indices;
	vector<Texture> textures;

	// Vertices
	for (unsigned i = 0; i < aiMesh->mNumVertices; i++) {
		// Coordinates
		vertices.push_back(aiMesh->mVertices[i].z);
		vertices.push_back(aiMesh->mVertices[i].y);
		vertices.push_back(aiMesh->mVertices[i].x);

		// Colors
		if (aiMesh->HasVertexColors(0)) {
			vertices.push_back(aiMesh->mColors[0][i].r);
			vertices.push_back(aiMesh->mColors[0][i].g);
			vertices.push_back(aiMesh->mColors[0][i].b);
		}
		else {
			vertices.push_back(0.0f);
			vertices.push_back(0.0f);
			vertices.push_back(0.0f);
		}

		// Textures Coords
		if (aiMesh->HasTextureCoords(0)) {
			vertices.push_back(aiMesh->mTextureCoords[0][i].y);
			vertices.push_back(aiMesh->mTextureCoords[0][i].x);
		}
		else {
			vertices.push_back(0.0f);
			vertices.push_back(0.0f);
		}

		// Normals
		vertices.push_back(aiMesh->mNormals[i].x);
		vertices.push_back(aiMesh->mNormals[i].y);
		vertices.push_back(aiMesh->mNormals[i].z);
	}

	// Indices
	for (unsigned i = 0; i < aiMesh->mNumFaces; i++) {
		const aiFace& face = aiMesh->mFaces[i];

		indices.push_back(face.mIndices[0]);
		indices.push_back(face.mIndices[1]);
		indices.push_back(face.mIndices[2]);
	}

	for(Texture tx : retrieveMeshTextures(pScene, aiMesh, path))
		textures.push_back(tx);

	return Mesh(vertices, indices, textures);
}

vector<unsigned> Scene::loadMesh(const char* path) {
	vector<unsigned> ids;

	Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

	if (!pScene) {
		cout << "Error while parsing" << endl;
		return ids;
	}

	for (int i = 0; i < pScene->mNumMeshes; i++)
		ids.push_back(addMesh(retrieveMesh(pScene, pScene->mMeshes[i], path)));

	return ids;
}

unsigned Scene::addLight(Light light) {
	lights.push_back(light);

	unsigned index = lights.size() - 1;

	setLightColor(index, light.getR(), light.getG(), light.getB(), light.getAlpha());

	return index;
}

void Scene::bindVertexPosition(unsigned vertexIndex, DFloat posX, DFloat posY, DFloat posZ) {
	VAO* vertex = &vertices.at(vertexIndex);

	vertex->bindPositionTo(posX, posY, posZ);
}

void Scene::bindVertexOrientation(unsigned vertexIndex, DFloat rotX, DFloat rotY, DFloat rotZ) {
	VAO* vertex = &vertices.at(vertexIndex);

	vertex->bindRotationTo(rotX, rotY, rotZ);
}

void Scene::translateVertex(unsigned vertexIndex, float xd, float yd, float zd) {
	translateOnShader(program, vertexIndex, xd, yd, zd);
}

void Scene::rotateVertex(unsigned vertexIndex, float xd, float yd, float zd) {
	vec3 euler(xd, yd, zd);
	quat quaterion = quat(euler);

	rotateOnShader(program, vertexIndex, quaterion.w, quaterion.x, quaterion.y, quaterion.z);
}

void Scene::scaleVertex(unsigned vertexIndex, float xd, float yd, float zd) {
	scaleOnShader(program, vertexIndex, xd, yd, zd);
}

void Scene::setLightColor(unsigned index, float red, float green, float blue, float alpha) {
	getLightShader(index)->activateShader();

	glUniform4f(getLightShader(index)->getUniformLocation("lightColor"), red, green, blue, alpha);
}

void Scene::setCameraMatrix(Camera* camera) {
	notifyCameraPosition(camera);

	activateShader();
	camera->sendMatrixToShader(program);

	for (Light light : lights) {
		light.getShader().activateShader();
		camera->sendMatrixToShader(light.getShader());
	}
}

void Scene::enableCulling() {
	glCullFace(GL_FRONT);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
}

void Scene::disableCulling() {
	glDisable(GL_CULL_FACE);
}
