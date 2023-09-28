#include "../include/igCollideHullBuilder.h"

void igCollideHullBuilder::addTriangle(igHullTriangle& triangle, Gap::igBool swapVertices)
{
	if (isRedundant(triangle))
		return;

	igHullTriangle tri = triangle;

	if (swapVertices)
	{
		tri.p1 = triangle.p2;
		tri.p2 = triangle.p1;
	}

	if (_optimizeHull)
		return;//TODO: optimize hull
	else
		_triangles->append(tri);
}

Gap::igBool igCollideHullBuilder::isRedundant(igHullTriangle& triangle) const
{
	Gap::igVec3f edge1 = triangle.p1 - triangle.p2;
	Gap::igVec3f edge2 = triangle.p3 - triangle.p2;
	
	edge1.cross(edge2);
	return edge1.length2() < 1.0e-8F;
}

Gap::igInt igCollideHullBuilder::getNodeCountFromTriangles()
{
	const Gap::igInt triangleCount = _triangles->getCount();

	if (triangleCount <= 0)
		return 0;

	Gap::igInt half = (triangleCount + 1) / 2;

	while (half > _maxTrianglesPerLeaf)
	{
		_levelCount++;
		half = (half + 1) / 2;
	}

	return (1 << (_levelCount + 1)) - 1;
}

struct sum_x
{
	bool operator()(const igHullTriangle& a, const igHullTriangle& b) const
	{
		return (b.p1[0] + b.p2[0] + b.p3[0]) > (a.p1[0] + a.p2[0] + a.p3[0]);
	}
};

struct sum_y
{
	bool operator()(const igHullTriangle& a, const igHullTriangle& b) const
	{
		return (b.p1[1] + b.p2[1] + b.p3[1]) > (a.p1[1] + a.p2[1] + a.p3[1]);
	}
};

struct sum_z
{
	bool operator()(const igHullTriangle& a, const igHullTriangle& b) const
	{
		return (b.p1[2] + b.p2[2] + b.p3[2]) > (a.p1[2] + a.p2[2] + a.p3[2]);
	}
};

void igCollideHullBuilder::process(Gap::igInt nodeIndex, Gap::igInt levelIndex, Gap::igInt triangleStartIndex, Gap::igInt triangleCount)
{
	static igHullNode defaultNode;

	defaultNode.min = 1.0e+30F;
	defaultNode.clipFlags1 = -1;
	defaultNode.max = -1.0e+30F;
	defaultNode.clipFlags2 = 0;

	while (true)
	{
		if (nodeIndex >= _nodes->getCount())
		{
			_nodes->resize(nodeIndex + 1, defaultNode);
		}

		igHullNode& node = _nodes->get(nodeIndex);

		if (triangleCount > 0)
		{
			for (Gap::igInt triangleIndex = triangleStartIndex; triangleIndex < (triangleStartIndex + triangleCount); triangleIndex++)
			{
				const igHullTriangle& triangle = _triangles->get(triangleIndex);
			
				node.clipFlags1 &= triangle.clipFlags;
				node.clipFlags2 |= triangle.clipFlags;
			
				node.min.makeMin(triangle.p1);
				node.max.makeMax(triangle.p1);
			
				node.min.makeMin(triangle.p2);
				node.max.makeMax(triangle.p2);
			
				node.min.makeMin(triangle.p3);
				node.max.makeMax(triangle.p3);
			}
		}

		if (levelIndex >= _levelCount)
			break;

		const Gap::igInt axis = levelIndex % 3;
		igHullTriangle* begin = &_triangles->getFirst();

		if (axis == 0)
		{
			std::make_heap(begin + triangleStartIndex, begin + (triangleStartIndex + triangleCount), sum_x());
			std::sort_heap(begin + triangleStartIndex, begin + (triangleStartIndex + triangleCount), sum_x());
		}
		else if (axis == 1)
		{
			std::make_heap(begin + triangleStartIndex, begin + (triangleStartIndex + triangleCount), sum_y());
			std::sort_heap(begin + triangleStartIndex, begin + (triangleStartIndex + triangleCount), sum_y());
		}
		else if (axis == 2)
		{
			std::make_heap(begin + triangleStartIndex, begin + (triangleStartIndex + triangleCount), sum_z());
			std::sort_heap(begin + triangleStartIndex, begin + (triangleStartIndex + triangleCount), sum_z());
		}

		const Gap::igInt half = (triangleCount + 1) / 2;

		process(2 * nodeIndex + 1, levelIndex + 1, triangleStartIndex, half);
		triangleCount -= half;
		triangleStartIndex += half;
		levelIndex++;
		nodeIndex = 2 * nodeIndex + 2;
	}
}

Gap::igBool igCollideHullBuilder::buildHull()
{
	if (_optimizeHull)
	{
		//TODO: optimize hull
	}
	
	const Gap::igInt triangleCount = _triangles->getCount();

	if (triangleCount <= 0)
		return false;

	const Gap::igInt nodeCount = getNodeCountFromTriangles();

	if (nodeCount <= 0)
		return false;

	_nodes->setCapacity(nodeCount + 1);

	process(0, 0, 0, triangleCount);

	igHullNode node;
	static Gap::igVec3f defaultBound;

	defaultBound = -900000.0F;
	node.min = defaultBound;
	node.max = defaultBound;

	for (Gap::igInt triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++)
	{
		igHullTriangle& triangle = _triangles->get(triangleIndex);

		for (Gap::igInt i = 0; i < 3; i++)
		{
			Gap::igVec3f& vert = triangle.getVertex(i);

			if (vert[2] < 60.0)
			{
				if (node.max == defaultBound)
					node.max = vert;

				node.max.makeMax(vert);

				if (node.min == defaultBound)
					node.min = vert;

				node.min.makeMin(vert);
			}
		}
	}

	_nodes->append(node);
	return true;
}