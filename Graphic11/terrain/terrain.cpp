
#include "stdafx.h"
#include "terrain.h"

using namespace std;
using namespace Gdiplus;
using namespace graphic;


cTerrain::cTerrain() :
	m_heightFactor(3.f)
,	m_isShowModel(true)
,	m_shader(NULL)
,	m_isRenderWater(true)
{
	m_rigids.reserve(32);

}

cTerrain::~cTerrain()
{
	Clear();
}


// *.TRN  ������ �о ������ �ʱ�ȭ �Ѵ�.
bool cTerrain::CreateFromTRNFile(cRenderer &renderer, const StrPath &fileName)
{
	sRawTerrain rawTerrain;
	if (!importer::ReadRawTerrainFile(fileName.c_str(), rawTerrain))
		return false;

	return CreateFromRawTerrain(renderer, rawTerrain);
}


// ���������� ���� ������ �����Ѵ�.
bool cTerrain::CreateFromRawTerrain(cRenderer &renderer, const sRawTerrain &rawTerrain)
{
	Clear();

	const StrPath mediaDir = cResourceManager::Get()->GetMediaDirectory();

	if (rawTerrain.heightMapStyle == 0)
	{
		// ���̸����� ������� �����̸�, ���� ���� �ε��Ѵ�.
		if (rawTerrain.heightMap.empty())
		{
			CreateTerrain( renderer, rawTerrain.rowCellCount, rawTerrain.colCellCount, 
				rawTerrain.cellSize, rawTerrain.textureFactor );
			CreateTerrainTexture(renderer, mediaDir+rawTerrain.bgTexture );
		}
		else
		{
			// �⺻ �������� ������� �����̸�, �⺻ ������ �����Ѵ�.
			CreateFromHeightMap( renderer, mediaDir+rawTerrain.heightMap, mediaDir+rawTerrain.bgTexture, 
				rawTerrain.heightFactor, rawTerrain.textureFactor, 
				rawTerrain.rowCellCount, rawTerrain.colCellCount, rawTerrain.cellSize );
		}
	}
	else if (rawTerrain.heightMapStyle == 1)
	{
		CreateFromGRDFormat(renderer, mediaDir+rawTerrain.heightMap.c_str(), mediaDir+rawTerrain.bgTexture.c_str(),
			rawTerrain.heightFactor, rawTerrain.textureFactor );
			//rawTerrain.rowCellCount, rawTerrain.colCellCount, rawTerrain.cellSize );
	}


	// ���̾� ����
	for (int i=0; i < MAX_LAYER; ++i)
	{
		if (rawTerrain.layer[ i].texture.empty())
			break;

		AddLayer();
		m_layer[ i].texture = cResourceManager::Get()->LoadTexture( 
			renderer, mediaDir+rawTerrain.layer[ i].texture );
	}


	// �� ����.
	cShader *modelShader = cResourceManager::Get()->LoadShader(renderer, "hlsl_skinning_using_texcoord_unlit.fx" );

	for (u_int i=0; i < rawTerrain.models.size(); ++i)
	{
		if (cModel *model = AddRigidModel(renderer, mediaDir+rawTerrain.models[ i].fileName))
		{
			model->SetTransform(rawTerrain.models[ i].tm);
			//model->SetShader(modelShader);
		}
	}

	m_alphaTexture.Create(renderer, mediaDir+rawTerrain.alphaTexture );

	m_isRenderWater = rawTerrain.renderWater;
	m_water.Create(renderer);
	m_skybox.Create(renderer,
		cResourceManager::Get()->FindFile("grassenvmap1024.dds"), 10000);

	return true;
}


bool cTerrain::CreateFromHeightMap(cRenderer &renderer, const StrPath &heightMapFileName,
	const StrPath &textureFileName, const float heightFactor, const float textureUVFactor,
	const int rowCellCount, const int colCellCount, const float cellSize)
	// heightFactor=3.f, textureUVFactor=1.f
	// rowCellCount=64, colCellCount=64, cellSize=50.f
{
	CreateTerrain(renderer, rowCellCount, colCellCount, cellSize, textureUVFactor);
	const bool result = UpdateHeightMap(renderer, heightMapFileName, textureFileName, heightFactor );
	return result;
}


// Grid ���� ���Ϸ� ������ �����Ѵ�.
// GRD ����: �׸����� ���� ���� �����ϴ� ���� ����.
bool cTerrain::CreateFromGRDFormat(cRenderer &renderer, const StrPath &gridFileName,
	const StrPath &textureFileName, const float heightFactor, const float textureUVFactor )
	// heightFactor=3.f, textureUVFactor=1.f
	// rowCellCount=64, colCellCount=64, cellSize=50.f
{
	Clear();

	if (!m_shader)
		m_shader = cResourceManager::Get()->LoadShader(renderer,  "hlsl_terrain_splatting.fx" );

	InitLayer(renderer);

	if (!m_grid.CreateFromFile(renderer, gridFileName))
		return false;

	m_grid.GetTexture().Create( renderer, textureFileName );

	m_heightFactor = heightFactor;
	m_heightMapFileName = gridFileName;

	return true;
}


// ���� �ؽ��� ����.
bool cTerrain::CreateTerrainTexture(cRenderer &renderer, const StrPath &textureFileName)
{
	m_grid.GetTexture().Clear();
	return m_grid.GetTexture().Create(renderer, textureFileName );
}


// ���� ����.
bool cTerrain::CreateTerrain(cRenderer &renderer, const int rowCellCount, const int colCellCount, const float cellSize
	,const float textureUVFactor)
	// rowCellCount=64, colCellCount=64, cellSize=50.f, textureUVFactor=1.f
{
	Clear();

	if (!m_shader)
		m_shader = cResourceManager::Get()->LoadShader(renderer,  "hlsl_terrain_splatting.fx" );

	InitLayer(renderer);

	m_grid.Create(renderer, rowCellCount, colCellCount, cellSize, textureUVFactor);

	m_skybox.Create( renderer,
		cResourceManager::Get()->FindFile("grassenvmap1024.dds"), 10000);

	m_water.Create(renderer);

	return true;
}


// �ؽ��� ���� ������ ���� ������ ä���.
// m_grid �� ������ ���¿��� �Ѵ�.
bool cTerrain::UpdateHeightMap(cRenderer &renderer, const StrPath &heightMapFileName,
	const StrPath &textureFileName, const float heightFactor )
{
	m_heightFactor = heightFactor;
	m_heightMapFileName = heightMapFileName;

	const wstring wfileName = common::str2wstr(heightMapFileName.c_str());
	Gdiplus::Bitmap bmp(wfileName.c_str());
	if (Gdiplus::Ok != bmp.GetLastStatus())
		return false;

	const int VERTEX_COL_COUNT = m_grid.GetColVertexCount();
	const int VERTEX_ROW_COUNT = m_grid.GetRowVertexCount();
	const float WIDTH = m_grid.GetWidth();
	const float HEIGHT = m_grid.GetHeight();

	const float incX = (float)(bmp.GetWidth()-1) / (float)m_grid.GetColCellCount();
	const float incY = (float)(bmp.GetHeight()-1) /(float)m_grid.GetRowCellCount();

	sVertexNormTex *pv = (sVertexNormTex*)m_grid.GetVertexBuffer().Lock();

	for (int i=0; i < VERTEX_COL_COUNT; ++i)
	{
		for (int k=0; k < VERTEX_ROW_COUNT; ++k)
		{
			sVertexNormTex *vtx = pv + (k*VERTEX_COL_COUNT) + i;

			Gdiplus::Color color;
			bmp.GetPixel((int)(i*incX), (int)(k*incY), &color);
			const float h = ((color.GetR() + color.GetG() + color.GetB()) / 3.f) 
				* heightFactor;
			vtx->p.y = h;
		}
	}

	m_grid.GetVertexBuffer().Unlock();

	m_grid.CalculateNormals();
	m_grid.GetTexture().Create( renderer, textureFileName );

	return true;
}



void cTerrain::PreRender(cRenderer &renderer)
{
	RET(!m_shader);
	RET(!m_isRenderWater);

	// Reflection plane in local space.
	Plane waterPlaneL(0,-1,0,0);

	// Reflection plane in world space.
	Matrix44 waterWorld;
	waterWorld.SetTranslate(Vector3(0,20,0)); // ���� ���� ���̴� 10������, �ø��� ���� 20���� ����
	Matrix44 WInvTrans;
	WInvTrans = waterWorld.Inverse();
	WInvTrans.Transpose();
	Plane waterPlaneW = waterPlaneL * WInvTrans;

	// Reflection plane in homogeneous clip space.
	Matrix44 WVPInvTrans = (waterWorld * GetMainCamera()->GetViewProjectionMatrix()).Inverse();
	WVPInvTrans.Transpose();
	Plane waterPlaneH = waterPlaneL * WVPInvTrans;

	float f[4] = {waterPlaneH.N.x, waterPlaneH.N.y, waterPlaneH.N.z, waterPlaneH.D};
	renderer.GetDevice()->SetClipPlane(0, (float*)f);
	renderer.GetDevice()->SetRenderState(D3DRS_CLIPPLANEENABLE, 1);

	m_water.BeginRefractScene(renderer);
	m_skybox.Render(renderer);
	RenderShader(renderer, *m_shader);
	m_water.EndRefractScene(renderer);

	// Seems like we need to reset these due to a driver bug.  It works
	// correctly without these next two lines in the REF and another 
	//video card, however.
	renderer.GetDevice()->SetClipPlane(0, (float*)f);
	renderer.GetDevice()->SetRenderState(D3DRS_CLIPPLANEENABLE, 1);
	renderer.GetDevice()->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

	m_water.BeginReflectScene(renderer);
	Matrix44 reflectMatrix = waterPlaneW.GetReflectMatrix();
	m_skybox.Render(renderer, reflectMatrix);
	RenderShader(renderer, *m_shader, reflectMatrix);
	m_water.EndReflectScene(renderer);

	renderer.GetDevice()->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
	renderer.GetDevice()->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
}


// ���� ���
void cTerrain::Render(cRenderer &renderer)
{
	if (m_shader)
	{
		// RenderShader() �Լ����� mVP �� �ʱ�ȭ �ϸ� �����Ÿ��� ������ ��Ÿ����
		// Render() �Լ����� �ϰ������� �ʱ�ȭ�ϰ� �ߴ�.
		cLightManager::Get()->GetMainLight().Bind(*m_shader);
		GetMainCamera()->Bind(*m_shader);

		//m_shader->SetMatrix( "g_mVP", GetMainCamera()->GetViewProjectionMatrix());
		//m_shader->SetVector( "g_vEyePos", GetMainCamera()->GetEyePos());
		//GetMainCamera()->Bind(*m_shader);
		m_shader->SetVector( "g_vFog", Vector3(1.f, 10000.f, 0)); // near, far

		m_skybox.Render(renderer);
		RenderShader(renderer, *m_shader);
		if (m_isRenderWater)
			m_water.Render(renderer);
	}
	else
	{
		m_grid.Render(renderer);
	}
}


void cTerrain::Move(const float elapseTime)
{
	if (m_isRenderWater)
		m_water.Update(elapseTime);
}


// ���̴��� ������ ����Ѵ�.
void cTerrain::RenderShader(cRenderer &renderer, cShader &shader, const Matrix44 &tm)
	// tm = Matrix44::Identity
{
	if (m_layer.empty())
	{
		shader.SetRenderPass(2);

		if (m_isShowModel && !m_rigids.empty())
			shader.SetRenderPass(2);
	}
	else
	{
		shader.SetTexture( "g_SplattingAlphaMap", m_alphaTexture );
		shader.SetFloat( "g_alphaUVFactor", GetTextureUVFactor() );

		const char* texName[] = {"Tex1", "Tex2", "Tex3", "Tex4" };
		for (u_int i=0; i < m_layer.size(); ++i)
			shader.SetTexture( texName[ i], *m_layer[ i].texture );
		for (u_int i=m_layer.size(); i < MAX_LAYER; ++i)
			shader.SetTexture( texName[ i], m_emptyTexture );

		shader.SetRenderPass(3);
		//if (m_isShowModel && !m_rigids.empty())
		//	shader.SetRenderPass(5);
	}

	if (m_isShowModel)
		RenderRigidModels(renderer, tm);


	shader.SetMatrix("g_mWorld", tm);
	m_grid.m_mtrl.Bind(shader);
	m_grid.m_tex.Bind(shader, "g_colorMapTexture");
	const int passCnt = shader.Begin();
	shader.BeginPass();
	shader.CommitChanges();
	m_grid.m_vtxBuff.Bind(renderer);
	m_grid.m_idxBuff.Bind(renderer);
	renderer.GetDevice()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 
		m_grid.m_vtxBuff.GetVertexCount(), 0, m_grid.m_idxBuff.GetFaceCount());
	shader.EndPass();
	shader.End();
	//m_grid.RenderShader(renderer, shader, tm);
}


// ���� �� ���
void cTerrain::RenderRigidModels(cRenderer &renderer, const Matrix44 &tm)
{
	for each (auto model in m_rigids)
	{
		model->Render(renderer, tm);
	}
}


// ���� �׸��ڸ� ������ ����Ѵ�.
void cTerrain::RenderModelShadow(cRenderer &renderer, cModel &model)
{
	model.UpdateShadow(renderer);

	Vector3 lightPos;
	Matrix44 view, proj, tt;
	cLightManager::Get()->GetMainLight().GetShadowMatrix(
		model.GetTransform().GetPosition(), lightPos, view, proj, tt );

	m_shader->SetTexture( "ShadowMap", model.GetShadow().GetTexture() );
	m_shader->SetMatrix( "mWVPT", view * proj * tt );
}


float Lerp(float p1, float p2, float alpha)
{
	return p1 * (1.f - alpha) + p2 * alpha;
}


// x/z��鿡�� ���� ��ǥ x,z ��ġ�� �ش��ϴ� ���� �� y�� �����Ѵ�.
float cTerrain::GetHeight(const float x, const float z)
{
	float newX = x + (m_grid.GetWidth() / 2.0f);
	float newZ = m_grid.GetHeight() - (z + (m_grid.GetHeight() / 2.0f));

	newX /= m_grid.GetCellSize();
	newZ /= m_grid.GetCellSize();

	const float col = ::floorf( newX );
	const float row = ::floorf( newZ );

	//  A   B
	//  *---*
	//  | / |
	//  *---*
	//  C   D
	const float A = GetHeightMapEntry( (int)row, (int)col );
	const float B = GetHeightMapEntry( (int)row, (int)col+1 );
	const float C = GetHeightMapEntry( (int)row+1, (int)col );
	const float D = GetHeightMapEntry( (int)row+1, (int)col+1 );

	const float dx = newX - col;
	const float dz = newZ - row;

	float height = 0.0f;
	if( dz < 1.0f - dx )  // upper triangle ABC
	{
		float uy = B - A; // A->B
		float vy = C - A; // A->C
		height = A + Lerp(0.0f, uy, dx) + Lerp(0.0f, vy, dz);
	}
	else // lower triangle DCB
	{
		float uy = C - D; // D->C
		float vy = B - D; // D->B
		height = D + Lerp(0.0f, uy, 1.0f - dx) + Lerp(0.0f, vy, 1.0f - dz);
	}

	return height;
}


// ���� 2���� �迭�� ���� ��, row, col �ε����� ���� ���� �����Ѵ�.
float cTerrain::GetHeightMapEntry( int row, int col )
{
	const int VERTEX_COL_COUNT = m_grid.GetColVertexCount();
	const int VERTEX_ROW_COUNT = m_grid.GetRowVertexCount();

	const int vtxSize = (VERTEX_COL_COUNT) * (VERTEX_ROW_COUNT);

	if( 0 > row || 0 > col )
		return 0.f;
	if( vtxSize <= (row * VERTEX_ROW_COUNT + col) ) 
		return 0.f;

	sVertexNormTex *pv = (sVertexNormTex*)m_grid.GetVertexBuffer().Lock();
	const float h = pv[ row * VERTEX_ROW_COUNT + col].p.y;
	m_grid.GetVertexBuffer().Unlock();
	return h;
}


// ���� ���� orig, dir �� �̿��ؼ�, �浹�� ���� y ��ǥ�� �����Ѵ�.
// ��ŷ ��ġ�� out�� �����ؼ� �����Ѵ�.
float cTerrain::GetHeightFromRay( const Vector3 &orig, const Vector3 &dir, OUT Vector3 &out )
{
	if (m_grid.Pick(orig, dir, out))
	{
		return GetHeight(out.x, out.z);
	}
	return 0.f;
}


// ��ŷ ó��.
bool cTerrain::Pick(const Vector3 &orig, const Vector3 &dir, OUT Vector3 &out)
{
	return m_grid.Pick(orig, dir, out);
}


// �� ��ŷ.
cModel* cTerrain::PickModel(const Vector3 &orig, const Vector3 &dir)
{
	for each (auto &model in m_rigids)
	{
		if (model->Pick(orig, dir))
			return model;
	}
	return NULL;
}


// �ʱ�ȭ.
void cTerrain::Clear()
{
	m_heightFactor = 3.f;
	m_heightMapFileName.clear();
	m_grid.Clear();

	for each (auto model in m_rigids)
	{
		SAFE_DELETE(model);
	}
	m_rigids.clear();
}


const StrPath& cTerrain::GetTextureName()
{
	return m_grid.GetTexture().GetTextureName();
}


// ���� �� �߰�
cModel* cTerrain::AddRigidModel(cRenderer &renderer, const cModel &model)
{
	RETV(FindRigidModel(model.GetId()), false); // already exist return

	m_rigids.push_back(model.Clone(renderer));
	return m_rigids.back();
}


// ���� �� �߰�
cModel* cTerrain::AddRigidModel(cRenderer &renderer, const StrPath &fileName)
{
	cModel *model = new cModel(common::GenerateId());
	if (!model->Create(renderer, fileName))
	{
		delete model;
		return NULL;
	}
	m_rigids.push_back(model);
	return model;
}


// ���� �� ã��.
cModel* cTerrain::FindRigidModel(const int id)
{
	for each (auto model in m_rigids)
	{
		if (model->GetId() == id)
			return model;
	}
	return NULL;
}


// ���� �� ����
// destruct : true �̸� �޸𸮸� �Ұ��Ѵ�.
bool cTerrain::RemoveRigidModel(cModel *model, const bool destruct) // destruct=true
{
	const bool result = common::removevector(m_rigids, model);
	if (destruct)
		SAFE_DELETE(model);
	return result;
}


// ���� �� ����.
bool cTerrain::RemoveRigidModel(const int id, const bool destruct) //destruct=true
{
	cModel *model = FindRigidModel(id);
	RETV(!model, false);
	return RemoveRigidModel(model, destruct);
}


void cTerrain::InitLayer(cRenderer &renderer)
{
	m_layer.clear();

	m_alphaTexture.Clear();
	m_alphaTexture.Create( renderer, ALPHA_TEXTURE_SIZE_W, ALPHA_TEXTURE_SIZE_H,
		D3DFMT_A8R8G8B8 );
}


// �ֻ��� ���̾� ����
sSplatLayer& cTerrain::GetTopLayer()
{
	if (m_layer.empty())
		m_layer.push_back(sSplatLayer());
	return m_layer.back();
}


// ���̾� �߰�.
bool cTerrain::AddLayer()
{
	if (m_layer.size() >= MAX_LAYER)
		return false;

	m_layer.push_back(sSplatLayer());
	return true;
}


// layer ��ġ�� ���̾ �����ϰ�, �������� �о� �ø���.
void cTerrain::DeleteLayer(int layer)
{
	RET(m_layer.empty());

	common::rotatepopvector(m_layer, (u_int)layer);

	// ����ִ� ���� �̹����� ���� �о�ø���.
	const DWORD delMask = GetAlphaMask(layer);
	DWORD moveMask = 0;
	for (u_int i=layer; i < m_layer.size(); ++i)
		moveMask |= GetAlphaMask(i+1);

	D3DLOCKED_RECT lockrect;
	m_alphaTexture.Lock(lockrect);

	BYTE *pbits = (BYTE*)lockrect.pBits;
	for (int ay=0; ay < ALPHA_TEXTURE_SIZE_H; ++ay)
	{
		for (int ax=0; ax < ALPHA_TEXTURE_SIZE_W; ++ax)
		{
			// A8R8G8B8 Format
			DWORD *ppixel = (DWORD*)(pbits + (ax*4) + (lockrect.Pitch * ay));
			DWORD moveVal = *ppixel & moveMask;
			*ppixel = *ppixel & ~(delMask | moveMask); // �̵��� AlphaTexture �ʱ�ȭ
			*ppixel = *ppixel | (moveVal << 8);
		}
	}

	m_alphaTexture.Unlock();
}


// layer �� �ش��ϴ� ����ũ�� �����Ѵ�.
DWORD cTerrain::GetAlphaMask(const int layer)
{
	return (0xFF << (24 - (layer * 8)));
}