#include "../include/igGenerateGlobalColor.h"

namespace Gap {

namespace Opt {

igBool igGenerateGlobalColor::configure(igInt sectionHandle)
{
	return true;
}

igMetaObject* igGenerateGlobalColor::getVisitedObjectMeta()
{
	return igGeometry::getClassMeta();
}

void igGenerateGlobalColor::visitor(igObject* object)
{
	igGeometry* geometry = static_cast<igGeometry*>(object);
	igInt attrCount = geometry->getAttrCount();
	igInt vertexColorAttrCount = 0;

	for (igInt i = 0; i < attrCount; i++)
	{
		igAttr* attr = geometry->getAttr(i);
		igBool hasVertexColors = false;

		if (attr->isOfType(igGeometryAttr::getClassMeta()))
			hasVertexColors = static_cast<igGeometryAttr*>(attr)->getVertexFormat().hasVertexColors();
		else if (attr->isOfType(igGeometryAttr2::getClassMeta()))
			hasVertexColors = static_cast<igGeometryAttr2*>(attr)->getVertexArray()->findVertexData(igVertexData::IG_VERTEX_COMPONENT_COLOR);

		if (hasVertexColors)
			vertexColorAttrCount++;
	}

	if (attrCount == vertexColorAttrCount)
		return;

	igInt parentCount = geometry->getParentCount();
	Gap::Attrs::igGlobalColorStateAttrRef globalColorState = Gap::Attrs::igGlobalColorStateAttr::instantiateRefFromPool(kIGMemoryPoolAttribute);

	globalColorState->setState(true);

	if (parentCount == 1 && geometry->getParent(0)->isOfType(igAttrSet::getClassMeta()))
	{
		igAttrSet* parentAttributes = static_cast<igAttrSet*>(geometry->getParent(0));

		if (parentAttributes->getAttrs()->findByMeta(Gap::Attrs::igGlobalColorStateAttr::getClassMeta()) < 0)
			parentAttributes->appendAttr(globalColorState);
	}
	else
	{
		igAttrSetRef attributes = igAttrSet::instantiateRefFromPool(kIGMemoryPoolAttribute);

		attributes->appendAttr(globalColorState);

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