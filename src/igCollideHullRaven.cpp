#include "../include/igCollideHullRaven.h"

namespace Gap
{

namespace OptExtension
{

igMatrix44f igCollideHullRaven::_geometryTM;
igInt igCollideHullRaven::_clipFlags = 507;
igInt igCollideHullRaven::_materialIndex = 0;

igBool igCollideHullRaven::configure(igInt sectionHandle)
{
	_builder->setInterface(getLogInterface());
	return true;
}

void removeGroupFromParents(igGroup* group)
{
	const igInt parentCount = group->getParentCount();

	for (igInt i = 0; i < parentCount; i++)
	{
		igGroup* parentGroup = igGroup::dynamicCast(group->getParent(i));

		if (parentGroup)
			parentGroup->removeChild(group);
	}
}

void removeInvisibleNodes(igNode* node)
{
	igGroup* group = igGroup::dynamicCast(node);

	if (group)
	{
		for (igInt i = group->getChildCount() - 1; i >= 0; i--)
			removeInvisibleNodes(group->getChild(i));

		if ((group->getFlags() & igNode::IS_INVISIBLE))
		{
			printf("Found Invisible igGroup node: %s\n", group->getName());
			removeGroupFromParents(group);
		}
	}
}

void removeRedundantAttrSets(igNode* node, igGroup* parent = NULL)
{
	if (!node->isOfType(igGroup::getClassMeta()))
		return;

	igGroup* group = static_cast<igGroup*>(node);

	if (parent && group->isOfExactType(igAttrSet::getClassMeta()) && !group->hasChildren())
	{
		parent->removeChild(group);
	}
	else
	{
		for (igInt i = 0; i < group->getChildCount(); i++)
			removeRedundantAttrSets(group->getChild(i), group);
	}
}

igBool igCollideHullRaven::processGeometry(igGeometry* geometry)
{
	const igInt attrCount = geometry->getAttrCount();

	if (attrCount <= 0)
		return false;

	igVec3f pos;
	igHullTriangle triangle;

	triangle.reserved = 0;
	triangle.clipFlags = _clipFlags;
	triangle.materialIndex = _materialIndex;

	for (igInt i = 0; i < attrCount; i++)
	{
		igAttr* attr = geometry->getAttr(i);
		igGeometryAttr1_5* mesh = igGeometryAttr1_5::dynamicCast(attr);
		igGeometryAttr2* mesh2 = igGeometryAttr2::dynamicCast(attr);

		if (!mesh && !mesh2)
			continue;

		const IG_GFX_DRAW primitiveType = mesh ? mesh->getPrimitiveType() : mesh2->getPrimitiveType();
		const igUnsignedInt primitiveCount = mesh ? mesh->getPrimitiveCount() : mesh2->getPrimitiveCount();

		if (primitiveType != IG_GFX_DRAW_TRIANGLES && primitiveType != IG_GFX_DRAW_TRIANGLE_STRIP)
			continue;

		igInt vertexIndex = 0;
		igVertexArray2* vertexArray2 = mesh2 ? mesh2->getVertexArray() : NULL;
		igIndexArray* indexArray = mesh ? mesh->getIndexArray() : mesh2->getIndexArray();

		for (igUnsignedInt j = 0; j < primitiveCount; j++)
		{
			for (igInt k = 0; k < 3; k++)
			{
				igVec3f& vertex = triangle.getVertex(k);
				igUnsignedInt index = vertexIndex++;

				if (indexArray)
					index = indexArray->getIndex(index);

				if (mesh)
					mesh->getPosition(index, pos);
				else
					igVertexArray2Helper::getPosition(vertexArray2, index, pos);

				pos.transformPoint(pos, _geometryTM);
				vertex = pos;
			}

			_builder->addTriangle(triangle);

			if (primitiveType == IG_GFX_DRAW_TRIANGLE_STRIP)
			{
				const igUnsignedInt primitiveLength = mesh ? mesh->getPrimitiveLength(j) : mesh2->getPrimitiveLength(j);
				igBool swapVertices = true;

				if (primitiveLength > 3)
				{
					for (igUnsignedInt k = 0; k < (primitiveLength - 3); k++)
					{
						igUnsignedInt index = vertexIndex++;

						if (indexArray)
							index = indexArray->getIndex(index);

						if (mesh)
							mesh->getPosition(index, pos);
						else
							igVertexArray2Helper::getPosition(vertexArray2, index, pos);

						pos.transformPoint(pos, _geometryTM);
						triangle.p1 = triangle.p2;
						triangle.p2 = triangle.p3;
						triangle.p3 = pos;
						swapVertices = !swapVertices;
						_builder->addTriangle(triangle, swapVertices);
					}
				}
			}
		}
	}

	return _builder->getTriangleCount() > 0;
}

void igCollideHullRaven::processUserInfo(igUserInfo* userInfo)
{
	const igString clipRules[31] = 
	{
		"stopwalk",
		"stopfly",
		"stopteleport",
		"stophero",
		"stopnpcneutral",
		"stopnpcenemy",
		"stopnpcaltenemy",
		"stopprojectile",
		"cameracollide",
		"forceonesided",
		"other1",
		"other2",
		"other3",
		"other4",
		"other5",
		"other6",
		"other7",
		"other8",
		"other9",
		"other10",
		"other11",
		"other12",
		"other13",
		"other14",
		"other15",
		"other16",
		"other17",
		"other18",
		"other19",
		"other20",
		"other21"
	};

	_materialIndex = 0;

	igStringKeyRef materialKey = igStringKey::instantiateRef();
	materialKey->setValue("material");

	igProperty* materialIndex = userInfo->getProperty(materialKey);

	if (materialIndex)
	{
		const igStringValue* materialIndexValue = igStringValue::dynamicCast(materialIndex->getValue());
		
		if (materialIndexValue)
		{
			igInt index = atoi(materialIndexValue->getValue());

			if (index > 31)
				index = 31;
			else if (index < 0)
				index = 0;

			_materialIndex = index;
		}
	}

	_clipFlags = 507;

	for (igInt i = 0; i < 31; i++)
	{
		igStringKeyRef clipRuleKey = igStringKey::instantiateRef();
		clipRuleKey->setValue(clipRules[i]);

		igProperty* clipRuleState = userInfo->getProperty(clipRuleKey);

		if (clipRuleState)
		{
			const igStringValue* clipRuleStateValue = igStringValue::dynamicCast(clipRuleState->getValue());
		
			if (clipRuleStateValue)
			{
				if (!stricmp(clipRuleStateValue->getValue(), "true"))
					_clipFlags |= 1 << i;
				else
					_clipFlags &= ~(1 << i);
			}
		}
	}
}

void igCollideHullRaven::processNode(igNode* node)
{
	if ((node->getFlags() & igNode::IS_DYNAMIC) != 0)
		return;

	igGroup* group = igGroup::dynamicCast(node);

	if (!group)
		return;

	const igMatrix44f lastGeometryTM(_geometryTM);
	const igInt lastClipFlags = _clipFlags;
	const igInt lastMaterialIndex = _materialIndex;

	if (node->isOfType(igTransform::getClassMeta()))
	{
		igTransform* transform = static_cast<igTransform*>(node);
		igTransformSequence* sequence = igTransformSequence::dynamicCast(transform->getTransformSource());

		if (sequence)
		{
			igMatrix44f* nodeTM;

			sequence->getMatrix(0, nodeTM);
			_geometryTM.multiply(*nodeTM);
		}
		else
		{
			_geometryTM.multiply(*transform->getMatrix());
		}
	}
	else if (node->isOfType(igGeometry::getClassMeta()))
	{
		if ((node->getFlags() & igNode::IS_COLLIDABLE))
		{
			if (!processGeometry(static_cast<igGeometry*>(node)))
				printf("Triangles from the Geometry '%s' have not been added to the collide hull\n", node->getName());
		}
	}
	else if (node->isOfType(igUserInfo::getClassMeta()))
	{
		processUserInfo(static_cast<igUserInfo*>(node));
	}

	for (igInt i = 0; i < group->getChildCount(); i++)
		processNode(group->getChild(i));

	_geometryTM.set(lastGeometryTM);
	_clipFlags = lastClipFlags;
	_materialIndex = lastMaterialIndex;
}

igBool igCollideHullRaven::apply(igNodeRef& node)
{
	_geometryTM.set(igMatrix44f::identityMatrix);
	
	processNode(node);

	if (_maxTrianglesPerLeaf < 4)
		_maxTrianglesPerLeaf = 4;

	_builder->setMaxTrianglesPerLeaf(_maxTrianglesPerLeaf);
	_builder->setOptimizeHull(_optimizeHull);
	
	if (!_builder->buildHull())
	{
		printf("Failed to build collide hull\n");
		return false;
	}

	printf("Collide Hull was built from %d Triangles and %d Nodes\n", _builder->getTriangleCount(), _builder->getNodeCount());

	/*for (int i = 0; i < _builder->getTriangleCount(); i++)
	{
		const igHullTriangle t = _builder->getTriangles()->get(i);
		printf("Tri %d has %d bytes: [%f,%f,%f] [%f,%f,%f] [%f,%f,%f] %d %d %d\n", i, sizeof(t), t.p1[0], t.p1[1], t.p1[2], t.p2[0], t.p2[1], t.p2[2], t.p3[0], t.p3[1], t.p3[2], t.reserved, t.clipFlags, t.materialIndex);
	}

	for (int i = 0; i < _builder->getNodeCount(); i++)
	{
		const igHullNode& t = _builder->getNodes()->get(i);
		printf("Node %d has %d bytes: [%f,%f,%f] [%f,%f,%f] %d %d \n", i, sizeof(t), t.min[0], t.min[1], t.min[2], t.max[0], t.max[1], t.max[2], t.clipFlags1, t.clipFlags2);
	}*/

	Raven::XMenSharedODL::igCollideHullRef collideHull = Raven::XMenSharedODL::igCollideHull::instantiateRef();
	igFloatList* nodes = collideHull->getNodes();
	igFloatList* triangles = collideHull->getTriangles();

	nodes->setCapacity(_builder->getTotalNodeBytes() / 4);
	nodes->setCount(_builder->getTotalNodeBytes() / 4);

	triangles->setCapacity(_builder->getTotalTriangleBytes() / 4);
	triangles->setCount(_builder->getTotalTriangleBytes() / 4);

	memcpy(nodes->getRawData(), _builder->getNodes()->getRawData(), _builder->getTotalNodeBytes());
	memcpy(triangles->getRawData(), _builder->getTriangles()->getRawData(), _builder->getTotalTriangleBytes());

	collideHull->setNodeCount(_builder->getNodeCount() - 1);
	collideHull->setTriangleCount(_builder->getTriangleCount());
	getInterface()->getFile()->getInfoList()->append(collideHull);

	_geometryTM.set(igMatrix44f::identityMatrix);
	removeInvisibleNodes(node);
	removeRedundantAttrSets(node, NULL);
	return true;
}

}

}