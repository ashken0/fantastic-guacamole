# fantastic-guacamole
����վ������ �� ����߽�
����վ���Ӧ��̩ɭ����� �� ��ÿ��������ཻ�Ķ���������ռ���������Ȩ��
����չʾ

In order to achieve accurate estimation of the spatial distribution of rainfall, it is
necessary to use interpolation methods,this is the area weighted Thiessen polygon method.

using wxWidgets gdal2.1.0 geos3.6.1

Edited geos-3.6.1\src\triangulate\quadedge\QuadEdgeSubdivision.cpp line 588 ,added the following :
	cellPoly->setUserData(reinterpret_cast<void*>(new Coordinate(c)));
	return cellPoly;
Edited geos-3.6.1\src\triangulate\VoronoiDiagramBuilder.cpp line 136 ,moved one line:
		else if(clipEnv.intersects(g->getEnvelopeInternal()))
		{
			result.reset( clipPoly->intersection(g) );
		}
		
		if(result.get() && !result->isEmpty() )
		{result->setUserData(((Geometry*)g)->getUserData()); // moved
			clipped->push_back(result.release());
		}