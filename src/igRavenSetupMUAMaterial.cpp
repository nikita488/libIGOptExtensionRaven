#include "../include/igRavenSetupMUAMaterial.h"

namespace Gap {

namespace Opt {

bool empty(igString str)
{
	return !str || strlen(str) <= 0;
}

igBool igRavenSetupMUAMaterial::configure(igInt sectionHandle)
{
	_regex->setRegularExpression(_diffuseMapName);
	return !empty(_diffuseMapName);
}

igMetaObject* igRavenSetupMUAMaterial::getVisitedObjectMeta()
{
	return igGeometry::getClassMeta();
}

igTextureBindAttr* findTextureAttribute(igAttrSet* attributes, igInt unitID = 0)
{
    for (igInt i = 0; i < attributes->getAttrCount(); i++)
    {
        igTextureBindAttr* attribute = igTextureBindAttr::dynamicCast(attributes->getAttr(i));
		
        if (attribute && attribute->getUnitID() == unitID)
            return attribute;
    }

    return NULL;
}

igBool hasTextureAttribute(igAttrSet* attributes, igInt unitID = 0)
{
	return findTextureAttribute(attributes, unitID) != NULL;
}

igImpStringIdRef igRavenSetupMUAMaterial::addTexture(igImpTexture texture)
{
	igImpStringIdRef id = igImpStringId::instantiateRefFromPool(getMemoryPool());
	
	id->setString(texture._name);
	return _sceneGraph->getTextureMapPool()->addTextureMap(texture, id) ? id : NULL;
}

igTextureBindAttrRef igRavenSetupMUAMaterial::getTextureBind(igImpStringIdRef id, igInt unitID)
{
	igTextureBindAttr* textureBind = _sceneGraph->getTextureMapPool()->getTexBind(id);
	
	if (!textureBind)
		return NULL;
	
	igTextureBindAttrRef copy = igTextureBindAttr::instantiateRefFromPool(kIGMemoryPoolAttribute);
	copy->setTexture(textureBind->getTexture());
	copy->setUnitID(unitID);
	return copy;
}

void igRavenSetupMUAMaterial::appendTextureMap(igAttrSet* attributes, igImpStringIdRef id, igInt unitID)
{
	if (!id || hasTextureAttribute(attributes, unitID))
		return;

	igTextureBindAttrRef textureBind = getTextureBind(id, unitID);
	igTextureStateAttr* textureState = _sceneGraph->getTextureMapPool()->getTextureEnable(unitID);

	if (!textureBind || !textureState)
		return;

	attributes->appendAttr(textureBind);
	attributes->appendAttr(textureState);
}

void igRavenSetupMUAMaterial::appendTextureMap(igAttrSet* attributes, igString texturePath, igInt unitID)
{
	appendTextureMap(attributes, addTexture(igImpTexture(texturePath)), unitID);
}

void igRavenSetupMUAMaterial::appendReflectionTextureMap(igAttrSet* attributes)
{
	if (hasTextureAttribute(attributes, 3))
		return;

	igImpStringId* textureID = _environmentMap->getTextureIdentifier();

	if (!textureID)
	{
		const igInt count = 6;
		const igImpEnvironmentChannel::CubeFace cubeFaces[count] = 
		{
			igImpEnvironmentChannel::FACE_RIGHT,
			igImpEnvironmentChannel::FACE_LEFT,
			igImpEnvironmentChannel::FACE_UP,
			igImpEnvironmentChannel::FACE_DOWN,
			igImpEnvironmentChannel::FACE_FRONT,
			igImpEnvironmentChannel::FACE_BACK
		};
		const igString faceTextures[count] = 
		{
			_reflectionMapRight, 
			_reflectionMapLeft, 
			_reflectionMapBack,
			_reflectionMapFront, 
			_reflectionMapUp, 
			_reflectionMapDown

		};
		const float faceRotations[count] = 
		{
			90.0F, 270.0F, 0.0F, 180.0F, 0.0F, 180.0F
		};
		igStringObjRef textureName = igStringObj::instantiateRefFromPool(kIGMemoryPoolTemporary);

		for (igInt i = 0; i < count; i++)
		{
			igImpTexture texture(faceTextures[i]);
		
			textureName->printf("%s.cube_%d", texture._fileName, i);
			texture.setName(textureName->getBuffer());
		
			igImpStringIdRef textureID = addTexture(texture);
		
			if (textureID)
				_environmentMap->setCubeFaceTexture(cubeFaces[i], textureID, faceRotations[i]);
		}

		_environmentMap->validate(_sceneGraph);
	}

	appendTextureMap(attributes, textureID, 3);

	if (empty(_reflectionMaskMap))
	{
		igShaderParametersAttrRef parameters = igShaderParametersAttr::instantiateRefFromPool(kIGMemoryPoolAttribute);
		igShaderConstantVectorRef reflectance = igShaderConstantVector::instantiateRefFromPool(getMemoryPool());

		reflectance->setName("material.reflectance");
		reflectance->setData(igVec4f(_reflectance, 0.0F, 0.0F, 0.0F));
		parameters->setUnitID(2);
		parameters->getDataList()->append(reflectance);
		attributes->appendAttr(parameters);
	}
	else
	{
		appendTextureMap(attributes, _reflectionMaskMap, 4);
	}
}

void generateTangentsAndBinormals(igGeometry* geometry)
{
	for (igInt i = 0; i < geometry->getAttrCount(); i++)
	{
		igAttr* attr = geometry->getAttr(i);
		igGeometryAttr2* ga2 = igGeometryAttr2::dynamicCast(attr);
		igGeometryAttr* ga = !ga2 ? igGeometryAttr::dynamicCast(attr) : NULL;
		igVertexArray2* va2 = ga2 ? ga2->getVertexArray() : NULL;
		const igVertexFormat* vf = !ga2 ? &ga->getVertexFormat() : NULL;

		if (!ga2 && !ga)
			continue;

		if (ga2)
		{
			if (va2->findVertexData(igVertexData::IG_VERTEX_COMPONENT_TANGENT) && va2->findVertexData(igVertexData::IG_VERTEX_COMPONENT_BINORMAL))
				continue;

			if (!va2->findVertexData(igVertexData::IG_VERTEX_COMPONENT_POSITION) || !va2->findVertexData(igVertexData::IG_VERTEX_COMPONENT_TEXCOORD))
				continue;

			Opt::igVertexTools::generateTangentsAndBinormals(ga2, 0);
		}
		else
		{
			if (vf->hasTangents() && vf->hasBinormals())
				continue;

			if (!vf->hasPositions() || vf->getTextureCoordCount() <= 0)
				continue;

			igVertexArrayHelperRef helper = igVertexArrayHelper::instantiateRef();
			helper->generateTangentsAndBinormals(ga, 0);
		}
	}
}

void igRavenSetupMUAMaterial::processGeometry(igGeometry* geometry, igAttrSet* attributes, igTextureBindAttr* diffuseTexture)
{
	if (!diffuseTexture)
		return;

	igStringObjRef textureName = igStringObj::instantiateRefFromPool(kIGMemoryPoolTemporary);

	textureName->set(diffuseTexture->getTexture()->getImage()->getName());
	textureName->removePathFromFileName();
	textureName->removeFileExtension();

	if (!_regex->test(textureName->getBuffer()))
		return;

	if (!empty(_normalMap))
	{
		appendTextureMap(attributes, _normalMap, 1);
		generateTangentsAndBinormals(geometry);
	}

	if (!empty(_specularMap))
		appendTextureMap(attributes, _specularMap, 2);

	if (!empty(_reflectionMapRight) && !empty(_reflectionMapLeft) && !empty(_reflectionMapBack) && !empty(_reflectionMapFront) && !empty(_reflectionMapUp) && !empty(_reflectionMapDown))
		appendReflectionTextureMap(attributes);

	if (!empty(_emissiveMap))
		appendTextureMap(attributes, _emissiveMap, 5);
}

void igRavenSetupMUAMaterial::visitor(igObject* object)
{
	igGeometry* geometry = igGeometry::dynamicCast(object);
	
	if (!geometry)
		return;

	int parentCount = geometry->getParentCount();

	if (parentCount > 1)
	{
		for (int i = 0; i < parentCount; i++)
		{
			igAttrSet* parent = igAttrSet::dynamicCast(geometry->getParent(i));
			
			if (!parent)
				continue;
			
			igAttrSetRef attributes = igAttrSet::instantiateRefFromPool(kIGMemoryPoolAttribute);

			processGeometry(geometry, attributes, findTextureAttribute(parent));
			parent->appendChild(attributes);
			parent->removeChild(geometry);
			attributes->appendChild(geometry);
		}
	}
	else if (parentCount == 1)
	{
		igAttrSet* parent = igAttrSet::dynamicCast(geometry->getParent(0));

		if (parent)
			processGeometry(geometry, parent, findTextureAttribute(parent));
	}
}

}

}