#include "Model.h"

#include <iostream>

namespace cat
{
	// CTOR & DTOR
	//--------------------

	Model::Model(Device& device,UniformBuffer<MatrixUbo>* ubo, const std::string& path)
		: m_Device{device},
		  m_pUniformBuffer{ubo},
		  m_Path(path), m_Directory{path}
	{
		LoadModel(path);

		// Create descriptor pool
		m_pDescriptorPool = new DescriptorPool(device);
		m_pDescriptorPool
			->AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_RawMeshes.size() * 2))
			->AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			              static_cast<uint32_t>(m_RawMeshes.size() * 2) * m_Material.amount);
		m_pDescriptorPool = m_pDescriptorPool->Create(static_cast<uint32_t>(m_RawMeshes.size() * 2));

		// Create descriptor set layout
		m_pDescriptorSetLayout = new DescriptorSetLayout(device);
		m_pDescriptorSetLayout = m_pDescriptorSetLayout
		                         ->AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                      VK_SHADER_STAGE_FRAGMENT_BIT) // albedo sampler
		                         ->AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                      VK_SHADER_STAGE_FRAGMENT_BIT) // normal sampler
		                         ->AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                      VK_SHADER_STAGE_FRAGMENT_BIT) // specular sampler
		                         ->Create();

		// Create meshes
		for (auto& data : m_RawMeshes)
		{
			m_Meshes.push_back(new Mesh(m_Device, ubo,
			                            m_pDescriptorSetLayout, m_pDescriptorPool,
			                            data.vertices, data.indices, data.material));
		}

		m_RawMeshes.clear();
	}

	Model::~Model()
	{
		for (auto mesh : m_Meshes)
		{
			delete mesh;
			mesh = nullptr;
		}

		delete m_pDescriptorPool;
		m_pDescriptorPool = nullptr;

		delete m_pDescriptorSetLayout;
		m_pDescriptorSetLayout = nullptr;
	}

	// Methods
	//--------------------
	void Model::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint16_t frameIdx, bool isDepthPass) const
	{
		for (auto& mesh : m_Meshes)
		{
			mesh->Bind(commandBuffer, pipelineLayout, frameIdx, isDepthPass);
			mesh->Draw(commandBuffer);
		}
	}

	// Private methods
	//--------------------
	void Model::LoadModel(const std::string& path)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path,
			aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_ConvertToLeftHanded
		);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cerr<<"ERROR::ASSIMP::" << std::string(importer.GetErrorString())<<std::endl;
			return;
		}
		m_Directory = path.substr(0, path.find_last_of('/'));

		ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f));
	}

	void Model::ProcessNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform)
	{
		glm::mat4 localTransform = ConvertMatrixToGLM(node->mTransformation);
		glm::mat4 globalTransform = parentTransform * localTransform;

		// for each mesh in the node
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			ProcessMesh(mesh, scene, globalTransform);
		}

		// for each of its children
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene, globalTransform);
		}
	}

	void Model::ProcessMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform)
	{
		std::vector<Mesh::Vertex> vertices;
		std::vector<uint32_t> indices;
		Mesh::Material material;


		// process vertices
		for (unsigned int i = 0; i< mesh->mNumVertices ; i++)
		{
			Mesh::Vertex vertex;
			glm::vec3 vector;

			// positions
			vector.x = mesh->mVertices[i].x ;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			glm::vec4 transformedPos = transform * glm::vec4(vector, 1.0f);
			vertex.pos = glm::vec3(transformedPos.x, transformedPos.y, transformedPos.z);

			// colors
			if (mesh->HasVertexColors(i))
			{
				vector.x = mesh->mColors[i][i].r;
				vector.y = mesh->mColors[i][i].g;
				vector.z = mesh->mColors[i][i].b;
				vertex.color = vector;
			}
			else
			{
				vertex.color = { 1.0f, 1.0f, 1.0f };
			}

			// uvs
			if (mesh->HasTextureCoords(0))
			{
				// uv
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.uv = vec;
			}
			else
			{
				vertex.uv = { 0.0f, 0.0f };
			}

			// normals
			if (mesh->HasNormals())
			{
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.normal = vector;
			}

			// tangents and bitangents
			if (mesh->HasTangentsAndBitangents())
			{
				// uv
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x; 
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.uv = vec;

				// tangent
				vector.x = mesh->mTangents[i].x;
				vector.y = mesh->mTangents[i].y;
				vector.z = mesh->mTangents[i].z;
				vertex.tangent = vector;

				// bitangent
				vector.x = mesh->mBitangents[i].x;
				vector.y = mesh->mBitangents[i].y;
				vector.z = mesh->mBitangents[i].z;
				vertex.bitangent = vector;
			}

			vertices.push_back(vertex);
		}

		// process indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
			{
				indices.push_back(face.mIndices[j]);
			}
		}

		// process materials
		if (scene->mNumMaterials > 0 && mesh->mMaterialIndex >= 0) {
			aiMaterial* aiMaterial = scene->mMaterials[mesh->mMaterialIndex];
			aiString path;

			if (aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
				material.albedoPath = m_Directory + "/" + path.C_Str() ;
			else
				material.albedoPath = "";

			if (aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS)
				material.normalPath = m_Directory + "/" + path.C_Str();
			else
				material.normalPath = "";

			if (aiMaterial->GetTexture(aiTextureType_METALNESS, 0, &path) == AI_SUCCESS ||
				aiMaterial->GetTexture(aiTextureType_SPECULAR, 0, &path) == AI_SUCCESS)
				material.specularPath = m_Directory + "/" + path.C_Str();
			else
				material.specularPath = "";
		}
	

		m_RawMeshes.emplace_back(Mesh::RawMeshData{
			vertices,
			indices,
			material
		});
	}

	glm::mat4 Model::ConvertMatrixToGLM(const aiMatrix4x4& mat) const
	{
		return glm::mat4(
			mat.a1, mat.b1, mat.c1, mat.d1,
			mat.a2, mat.b2, mat.c2, mat.d2,
			mat.a3, mat.b3, mat.c3, mat.d3,
			mat.a4, mat.b4, mat.c4, mat.d4
		);
	}

}
