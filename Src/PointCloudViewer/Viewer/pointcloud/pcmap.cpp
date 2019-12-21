
#include "stdafx.h"
#include "pcmap.h"


cPointCloudMap::cPointCloudMap()
	: m_res(0,0)
	, m_snap(0,0)
	, m_map(nullptr)
	, m_pcd(nullptr)
	, m_pcCount(0)
{
}

cPointCloudMap::~cPointCloudMap()
{
	Clear();
}


bool cPointCloudMap::Read(const StrPath &fileName
	, const int resolutionWidth //= 300
	, const int resolutionHeight //= 200
)
{
	Clear();

	common::cSimpleData sdata;
	if (!sdata.Read(fileName.c_str(), " "))
		return false;

	m_res = { resolutionWidth, resolutionHeight };
	m_snap = { 10000 / resolutionWidth,  10000 / resolutionHeight };	
	if ((m_snap.x <= 0) || (m_snap.y <= 0))
		return false;

	const int mapSize = resolutionWidth * resolutionHeight;
	m_map = new Vector3[mapSize];
	ZeroMemory(m_map, sizeof(float)*mapSize);

	if (!sdata.m_table.empty() && !sdata.m_table[0].empty())
	{
		m_pcCount = atoi(sdata.m_table[0][0].c_str());
		m_pcd = new Vector3[m_pcCount];
		m_uvPos = new sUV[m_pcCount];
	}

	const int w = resolutionWidth;
	const int h = resolutionHeight;
	uint duplicateCnt = 0;
	Vector3 *pcd = m_pcd;
	sUV *uvP = m_uvPos;

	for (auto &row : sdata.m_table)
	{
		if (row.size() < 5)
			continue;

		const Vector2 uv((float)atof(row[0].c_str()), (float)atof(row[1].c_str()));
		// opengl space
		const Vector3 opos((float)atof(row[2].c_str())
			, (float)atof(row[3].c_str())
			, (float)atof(row[4].c_str()));
		const Vector3 pos(opos.x, opos.y, opos.z);

		if (uv.x == 0.f)
		{
			int a = 0;
		}

		common::Vector2i mpos = GetUV2MPos(uv);
		Vector3 *p = m_map + (mpos.y * w) + mpos.x;
		if (!p->IsEmpty())
			++duplicateCnt;
		*p = pos;
		*pcd++ = pos;
		uvP->uv = uv;
		uvP->pos = pos;
		++uvP;
		m_uvSearchX.insert(uv.x);
	}

	//Interpolation();

	return true;
}


// get point cloud position from correspond uv coordiate
Vector3 cPointCloudMap::GetPosition(const Vector2 &uv)
{
	RETV(!m_map, Vector3(0, 0, 0));

	// map search
	//common::Vector2i mpos = GetUV2MPos(uv);
	//Vector3 *p = m_map + (mpos.y * m_res.x) + mpos.x;
	//return *p;

	// array search
	float minD = FLT_MAX;
	float uvx = 0.f;
	for (auto x : m_uvSearchX)
	{
		const float d = abs(x - uv.x);
		if (d < minD)
		{
			minD = d;
			uvx = x;
		}
	}

	minD = FLT_MAX;
	float uvy = 0.f;
	int minIdx = -1;
	for (uint i = 0; i < m_pcCount; ++i)
	{
		if (uvx == m_uvPos[i].uv.x)
		{
			const float d = abs(m_uvPos[i].uv.y - uv.y);
			if (d < minD)
			{
				minD = d;
				uvy = m_uvPos[i].uv.y;
				minIdx = i;
			}
		}
	}

	if (minIdx < 0)
		return Vector3::Zeroes;

	return m_uvPos[minIdx].pos;
}


// m_map 에서 빠진부부분을 주변데이타로 메꾼다.
void cPointCloudMap::Interpolation()
{
	RET(!m_map);

	int cnt1 = 0;
	for (int i = 0; i < m_res.x; ++i) // width
	{
		for (int k = 70; k < m_res.y-70; ++k) // height
		{
			Vector3 *p = m_map + (k * m_res.x) + i;
			// 위아래를 검색해서 가까운 쪽의 데이타로 채운다.
			if (p->IsEmpty())
			{
				++cnt1;

				for (int m = 1; m < 5; ++m)
				{
					Vector3 *p0 = m_map + (min(m_res.y, k + m) * m_res.x) + i;
					Vector3 *p1 = m_map + (max(0, k - m) * m_res.x) + i;

					if (!p0->IsEmpty())
					{
						*p = *p0;
						break;
					}
					if (!p1->IsEmpty())
					{
						*p = *p1;
						break;
					}
				}
			}

			if (p->IsEmpty())
			{

				// 좌우를 검색해서 가까운 쪽의 데이타로 채운다.
				// 좌우 5px 검색
				for (int m = 1; m < 5; ++m)
				{
					Vector3 *p0 = m_map + (k * m_res.x) + min(m_res.x, i + m);
					Vector3 *p1 = m_map + (k * m_res.x) + max(0, i - m);
					if (!p0->IsEmpty())
					{
						*p = *p0;
						break;
					}
					if (!p1->IsEmpty())
					{
						*p = *p1;
						break;
					}
				}
			}
		}
	}
	
	int cnt2 = 0;
	for (int i = 0; i < m_res.x; ++i) // width
	{
		for (int k = 50; k < m_res.y - 50; ++k) // height
		{
			Vector3 *p = m_map + (k * m_res.x) + i;
			if (p->IsEmpty())
				++cnt2;
		}
	}

	int a = 0;
}


// convert uv coordinate position to map position
common::Vector2i cPointCloudMap::GetUV2MPos(const Vector2 &uv)
{
	RETV(!m_map, common::Vector2i(0, 0));

	const int x = (uint)(uv.x * 10000.f);
	const int y = (uint)(uv.y * 10000.f);
	const int i = max(0, min(m_res.x - 1, x / m_snap.x - 1));
	const int k = max(0, min(m_res.y - 1, y / m_snap.y - 1));
	return common::Vector2i(i, k);
}


void cPointCloudMap::Clear()
{
	m_pcCount = 0;
	SAFE_DELETEA(m_map);
	SAFE_DELETEA(m_pcd);
	SAFE_DELETEA(m_uvPos);
	m_uvSearchX.clear();
}
