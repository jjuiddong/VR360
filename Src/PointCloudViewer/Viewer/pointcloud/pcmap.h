//
// 2019-11-23, jjuiddong
// point cloud position map
//		- uv coord - correspond point cloud position (xyz)
//
// file format
//		- data size
//		- uv coord <x> <y> point cloude position <x> <y> <z>
//		- uv coord range (0 ~ 1)
//
#pragma once


class cPointCloudMap
{
public:
	cPointCloudMap();
	virtual ~cPointCloudMap();

	bool Read(const StrPath &fileName
		, const int resolutionWidth = 300
		, const int resolutionHeight = 200
	);
	Vector3 GetPosition(const Vector2 &uv);
	void Clear();


protected:
	void Interpolation();
	common::Vector2i GetUV2MPos(const Vector2 &uv);


public:
	Vector3 *m_map; // point cloud project data
					// size = width x height 
					// m_map[x*w + y] = point cloud position

	Vector3 *m_pcd; // point cloud data
	uint m_pcCount; // point cloud count

	struct sUV {
		Vector2 uv;
		Vector3 pos;
	};
	sUV *m_uvPos; // point cloud + uv positions
	set<float> m_uvSearchX;

	common::Vector2i m_res; // resolution
	common::Vector2i m_snap;
};
