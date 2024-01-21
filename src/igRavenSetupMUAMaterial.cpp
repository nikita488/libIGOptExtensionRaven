#include "../include/igRavenSetupMUAMaterial.h"

namespace Gap
{

namespace Opt
{

igTextureBindAttr* findTextureBindAttr(igAttrSet* attrSet, igInt unitID = 0)
{
	for (igNode* currentNode = attrSet; currentNode->getParentCount() > 0; currentNode = currentNode->getParent(0))
	{
		if (currentNode->isOfType(igAttrSet::getClassMeta()))
		{
			attrSet = static_cast<igAttrSet*>(currentNode);

			for (igInt i = 0; i < attrSet->getAttrCount(); i++)
			{
				igTextureBindAttr* textureBind = igTextureBindAttr::dynamicCast(attrSet->getAttr(i));

				if (textureBind && textureBind->getUnitID() == unitID)
					return textureBind;
			}
		}
	}

	return NULL;
}

igBool hasTextureBindAttr(igAttrSet* attrSet, igInt unitID = 0)
{
	return findTextureBindAttr(attrSet, unitID) != NULL;
}

igBool generateTangentsAndBinormals(igGeometry* geometry)
{
	for (igInt i = 0; i < geometry->getAttrCount(); i++)
	{
		igAttr* attr = geometry->getAttr(i);

		if (attr->isOfType(igGeometryAttr::getClassMeta()))
		{
			igGeometryAttr* geometry = static_cast<igGeometryAttr*>(attr);
			const igVertexFormat& format = geometry->getVertexFormat();

			if (format.hasPositions() && format.hasNormals() && format.getTextureCoordCount() > 0 && !format.hasTangents() && !format.hasBinormals())
			{
				igVertexArrayHelperRef helper = igVertexArrayHelper::instantiateRef();
				helper->generateTangentsAndBinormals(geometry, 0, true);
				return true;
			}
		}
		else if (attr->isOfType(igGeometryAttr2::getClassMeta()))
		{
			igGeometryAttr2* geometry = static_cast<igGeometryAttr2*>(attr);
			igVertexArray2* vertices = geometry->getVertexArray();

			if (vertices &&
				vertices->findVertexData(igVertexData::IG_VERTEX_COMPONENT_POSITION) != NULL &&
				vertices->findVertexData(igVertexData::IG_VERTEX_COMPONENT_NORMAL) != NULL &&
				vertices->findVertexData(igVertexData::IG_VERTEX_COMPONENT_TEXCOORD) != NULL &&
				vertices->findVertexData(igVertexData::IG_VERTEX_COMPONENT_TANGENT) == NULL &&
				vertices->findVertexData(igVertexData::IG_VERTEX_COMPONENT_BINORMAL) == NULL)
			{
				Opt::igVertexTools::generateTangentsAndBinormals(geometry, 0, true);
				return true;
			}
		}
	}

	return false;
}

igBool igRavenSetupMUAMaterial::configure(igInt sectionHandle)
{
	if (_diffuseMapName != NULL && strlen(_diffuseMapName) > 0)
	{
		_regex->setRegularExpression(_diffuseMapName);
		return true;
	}

	printf("The name of the Diffuse Map has not been set\n");
	return false;
}

igMetaObject* igRavenSetupMUAMaterial::getVisitedObjectMeta()
{
	return igGeometry::getClassMeta();
}

igBool igRavenSetupMUAMaterial::addReflectionTextureMaps(igGeometry* geometry, igAttrSet* attrs)
{
	const igInt count = 6;
	const igString faceTextures[count] = 
	{
		_reflectionMapRight, 
		_reflectionMapLeft, 
		_reflectionMapBack,
		_reflectionMapFront, 
		_reflectionMapUp, 
		_reflectionMapDown
	};

	for (igInt i = 0; i < count; i++)
	{
		igString faceTexture = faceTextures[i];

		if (faceTexture == NULL || strlen(faceTexture) <= 0) {
			return false;
		}
	}

	if (hasTextureBindAttr(geometry, 3))
	{
		printf("igTextureBindAttr with unitID 3 already exists on igGeometry `%s`\n", geometry->getName());
		return false;
	}

	igImpStringId* textureID = _environmentMap->getTextureIdentifier();

	if (!textureID)
	{
		igImpTextureMapBuilder* textureMaps = _sceneGraph->getTextureMapPool();
		const igImpEnvironmentChannel::CubeFace cubeFaces[count] = 
		{
			igImpEnvironmentChannel::FACE_RIGHT,
			igImpEnvironmentChannel::FACE_LEFT,
			igImpEnvironmentChannel::FACE_UP,
			igImpEnvironmentChannel::FACE_DOWN,
			igImpEnvironmentChannel::FACE_FRONT,
			igImpEnvironmentChannel::FACE_BACK
		};
		const float faceRotations[count] = 
		{
			90.0F, 270.0F, 0.0F, 180.0F, 0.0F, 180.0F
		};
		igStringObjRef textureName = igStringObj::instantiateRef();

		for (igInt i = 0; i < count; i++)
		{
			igImpStringIdRef faceTextureID = igImpStringId::instantiateRef();

			faceTextureID->setString(faceTextures[i]);
			
			if (!textureMaps->getTextureMap(faceTextureID))
			{
				const igString texturePath = faceTextureID->getString();
				igImpTexture& texture = igImpTexture(texturePath);
		
				textureName->printf("%s.cube_%d", texturePath, i);
				texture.setName(textureName->getBuffer());

				if (!textureMaps->addTextureMap(texture, faceTextureID))
				{
					printf("Failed to load texture image `%s` for cube face %d\n", texturePath, i);
					return false;
				}
			}

			_environmentMap->setCubeFaceTexture(cubeFaces[i], faceTextureID, faceRotations[i]);
		}

		_environmentMap->validate(_sceneGraph);
		textureID = _environmentMap->getTextureIdentifier();
		
		if (textureID)
		{
			textureMaps->getTexBind(textureID)->setUnitID(3);
		}
		else
		{
			printf("Failed to create igTextureCubeAttr for igGeometry `%s`\n", geometry->getName());
			return false;
		}
	}

	printf("Adding igTextureBindAttr `%s` with unitID 3 to igGeometry `%s`\n", textureID->getString(), geometry->getName());
	addTextureMap(textureID, attrs);

	if (!addTextureMap(geometry, _reflectionMaskMap, 4, attrs) && !hasTextureBindAttr(geometry, 4))
	{
		printf("Adding igShaderParametersAttr with Reflectance %f to igGeometry `%s`\n", _reflectance, geometry->getName());
		
		igShaderParametersAttrRef parameters = igShaderParametersAttr::instantiateRef();
		igShaderConstantVectorRef reflectance = igShaderConstantVector::instantiateRef();

		reflectance->setName("material.reflectance");
		reflectance->setData(igVec4f(_reflectance, 0.0F, 0.0F, 0.0F));
		parameters->setUnitID(2);
		parameters->getDataList()->append(reflectance);
		attrs->appendAttr(parameters);
	}

	return true;
}

igBool igRavenSetupMUAMaterial::addTextureMap(igGeometry* geometry, igString texturePath, igInt unitID, igAttrSet* attrs)
{
	if (texturePath == NULL || strlen(texturePath) <= 0)
	{
		return false;
	}

	if (unitID < 0 || unitID >= 8)
	{
		printf("Invalid unitID. Only unitID's 0..7 are supported\n");
		return false;
	}

	if (hasTextureBindAttr(geometry, unitID))
	{
		printf("igTextureBindAttr with unitID %d already exists on igGeometry `%s`\n", unitID, geometry->getName());
		return false;
	}

	igImpTextureMapBuilder* textureMaps = _sceneGraph->getTextureMapPool();
	igImpStringIdRef id = igImpStringId::instantiateRef();
	
	id->setString(texturePath);

	if (!textureMaps->getTextureMap(id))
	{
		igImpTexture& texture = igImpTexture(texturePath);
		
		if (!textureMaps->addTextureMap(texture, id))
		{
			printf("Failed to load texture image `%s` for unitID %d\n", texturePath, unitID);
			return false;
		}
		else
		{
			textureMaps->getTexBind(id)->setUnitID(unitID);
		}
	}

	printf("Adding igTextureBindAttr `%s` with unitID %d to igGeometry `%s`\n", texturePath, unitID, geometry->getName());
	addTextureMap(id, attrs);
	return true;
}

void igRavenSetupMUAMaterial::addTextureMap(igImpStringId* textureID, igAttrSet* attrs)
{
	igImpTextureMapBuilder* textureMaps = _sceneGraph->getTextureMapPool();
	igTextureBindAttr* textureBind = textureMaps->getTexBind(textureID);

	if (textureBind)
	{
		attrs->appendAttr(textureBind);
		attrs->appendAttr(textureMaps->getTextureEnable(textureBind->getUnitID()));
	}
}

igBool igRavenSetupMUAMaterial::addTextureMaps(igGeometry* geometry, igAttrSet* attrs)
{
	igTextureBindAttr* diffuseTextureBind = findTextureBindAttr(geometry);

	if (diffuseTextureBind == NULL)
	{
		printf("igTextureBindAttr with unitID 0 not found for igGeometry `%s`", geometry->getName());
		return false;
	}

	igStringObjRef textureName = igStringObj::instantiateRef();

	textureName->set(diffuseTextureBind->getTexture()->getImage()->getName());
	textureName->removePathFromFileName();
	textureName->removeFileExtension();

	if (!_regex->test(textureName->getBuffer()))
		return false;

	const igInt attrsCount = attrs->getAttrCount();

	if (addTextureMap(geometry, _normalMap, 1, attrs))
		if (!generateTangentsAndBinormals(geometry))
			printf("Failed to generate tangets and binormals for igGeometry `%s`\n", geometry->getName());

	addTextureMap(geometry, _specularMap, 2, attrs);
	addReflectionTextureMaps(geometry, attrs);
	addTextureMap(geometry, _emissiveMap, 5, attrs);
	return attrs->getAttrCount() != attrsCount;
}

void igRavenSetupMUAMaterial::visitor(igObject* object)
{
	igGeometry* geometry = static_cast<igGeometry*>(object);
	const igInt attrCount = geometry->getAttrCount();
	const igInt parentCount = geometry->getParentCount();

	if (parentCount == 1 && geometry->getParent(0)->isOfType(igAttrSet::getClassMeta()))
	{
		igAttrSet* parentAttributes = static_cast<igAttrSet*>(geometry->getParent(0));
		addTextureMaps(geometry, parentAttributes);
	}
	else
	{
		igAttrSetRef attributes = igAttrSet::instantiateRef();

		if (addTextureMaps(geometry, attributes))
		{
			for (igInt i = 0; i < parentCount; i++)
			{
				igGroup* parent = igGroup::dynamicCast(geometry->getParent(i));

				if (parent)
				{
					parent->appendChild(attributes);
					parent->removeChild(geometry);
				}
			}

			attributes->appendChild(geometry);
		}
	}
}

}

}