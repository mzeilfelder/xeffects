#ifndef H_XEFFECTS_CB
#define H_XEFFECTS_CB

#include "EffectHandler.h"

using namespace irr;
using namespace scene;
using namespace video;
using namespace core;

// Helper defines to use faster setVertexShaderConstant functions which got added in Irrlicht 1.9
// Sadly does not work so far. The problem is that the new functions need to have one callback instance 
// per shader material (shader constants are always per shader).
// Depth and shadow callback will need some rewrite to make that work (nearly seems to work, but check console errors, especially on OpenGL)
//#define USE_1_9_SHADER_CONSTANTS
#if defined(USE_1_9_SHADER_CONSTANTS) && (IRRLICHT_VERSION_MAJOR > 1 || (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR >=9))
	#define XE_SET_VERTEX_SHADER_CONSTANT(id_var, name, pointer, count) \
		if ( id_var < -1 ) \
			id_var = services->getVertexShaderConstantID(name); \
		services->setVertexShaderConstant(id_var, pointer, count);
#else
	#define XE_SET_VERTEX_SHADER_CONSTANT(id_var, name, pointer, count) \
		services->setVertexShaderConstant(name, pointer, count);
#endif

#if defined(USE_1_9_SHADER_CONSTANTS) && (IRRLICHT_VERSION_MAJOR > 1 || (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR >=9))
	#define XE_SET_PIXEL_SHADER_CONSTANT(id_var, name, pointer, count) \
	if ( id_var < -1 ) \
		id_var = services->getPixelShaderConstantID(name); \
	services->setPixelShaderConstant(id_var, pointer, count);
#else
	#define XE_SET_PIXEL_SHADER_CONSTANT(id_var, name, pointer, count) \
		services->setPixelShaderConstant(name, pointer, count);
#endif

class DepthShaderCB : public video::IShaderConstantSetCallBack
{
public:
	DepthShaderCB(EffectHandler* effectIn) 
		: effect(effectIn) 
		, worldViewProjId(-2)
		, maxDId(-2)
	{};

	virtual void OnSetConstants(video::IMaterialRendererServices* services, s32 userData)
	{
		IVideoDriver* driver = services->getVideoDriver();

		core::matrix4 worldViewProj = driver->getTransform(video::ETS_PROJECTION);			
		worldViewProj *= driver->getTransform(video::ETS_VIEW);
		worldViewProj *= driver->getTransform(video::ETS_WORLD);

		XE_SET_VERTEX_SHADER_CONSTANT(worldViewProjId, "mWorldViewProj", worldViewProj.pointer(), 16);

		XE_SET_VERTEX_SHADER_CONSTANT(maxDId, "MaxD", &FarLink, 1);
	}

	f32 FarLink;

private:
	EffectHandler* effect;

	irr::s32 worldViewProjId;
	irr::s32 maxDId;
};

class ShadowShaderCB : public video::IShaderConstantSetCallBack
{
public:
	ShadowShaderCB(EffectHandler* effectIn) : effect(effectIn) 
	,worldViewProjId(-2)
	,worldViewProj2Id(-2)
	,lightPosId(-2)
	,maxDId(-2)
	,mapResId(-2)
	,lightColourId(-2)
	{};

	virtual void OnSetMaterial(const SMaterial& material) {}

	virtual void OnSetConstants(video::IMaterialRendererServices* services, s32 userData)
	{
		IVideoDriver* driver = services->getVideoDriver();

		matrix4 worldViewProj = driver->getTransform(video::ETS_PROJECTION);			
		worldViewProj *= driver->getTransform(video::ETS_VIEW);
		worldViewProj *= driver->getTransform(video::ETS_WORLD);
		XE_SET_VERTEX_SHADER_CONSTANT(worldViewProjId, "mWorldViewProj", worldViewProj.pointer(), 16);

		worldViewProj = ProjLink;			
		worldViewProj *= ViewLink;
		worldViewProj *= driver->getTransform(video::ETS_WORLD);
		XE_SET_VERTEX_SHADER_CONSTANT(worldViewProj2Id, "mWorldViewProj2", worldViewProj.pointer(), 16);

		driver->getTransform(video::ETS_WORLD).getInverse(invWorld);
		vector3df lightPosOS = LightLink;
		invWorld.transformVect(lightPosOS); 
		XE_SET_VERTEX_SHADER_CONSTANT(lightPosId, "LightPos", reinterpret_cast<f32*>(&lightPosOS.X), 4);
		
		XE_SET_VERTEX_SHADER_CONSTANT(maxDId, "MaxD", reinterpret_cast<f32*>(&FarLink), 1);
		XE_SET_VERTEX_SHADER_CONSTANT(mapResId, "MAPRES", &MapRes, 1);

		XE_SET_PIXEL_SHADER_CONSTANT(lightColourId, "LightColour", reinterpret_cast<f32*>(&LightColour.r), 4);
	}

	video::SColorf LightColour;
	core::matrix4 ProjLink;
	core::matrix4 ViewLink;
	core::vector3df LightLink;
	f32 FarLink, MapRes;

private:
	EffectHandler* effect;
	core::matrix4 invWorld;

	irr::s32 worldViewProjId;
	irr::s32 worldViewProj2Id;
	irr::s32 lightPosId;
	irr::s32 maxDId;
	irr::s32 mapResId;
	irr::s32 lightColourId;
};


class ScreenQuadCB : public irr::video::IShaderConstantSetCallBack
{
public:
	ScreenQuadCB(EffectHandler* effectIn, bool defaultV = true) 
		: effect(effectIn), defaultVertexShader(defaultV) 
	,colorMapSamplerId(-2)
	,screenMapSamplerId(-2)
	,depthMapSamplerId(-2)
	,userMapSamplerId(-2)
	,screenXId(-2)
	,screenYId(-2)
	,lineStarts0Id(-2)
	,lineStarts1Id(-2)
	,lineStarts2Id(-2)
	,lineStarts3Id(-2)
	,lineEnds0Id(-2)
	,lineEnds1Id(-2)
	,lineEnds2Id(-2)
	,lineEnds3Id(-2)
	{};

	virtual void OnSetConstants(irr::video::IMaterialRendererServices* services, irr::s32 userData)
	{
		if(services->getVideoDriver()->getDriverType() == irr::video::EDT_OPENGL)
		{
			irr::s32 TexVar = 0;
			XE_SET_PIXEL_SHADER_CONSTANT(colorMapSamplerId, "ColorMapSampler", &TexVar, 1); 

			TexVar = 1;
			XE_SET_PIXEL_SHADER_CONSTANT(screenMapSamplerId, "ScreenMapSampler", &TexVar, 1); 

			TexVar = 2;
			XE_SET_PIXEL_SHADER_CONSTANT(depthMapSamplerId, "DepthMapSampler", &TexVar, 1); 

			TexVar = 3;
			XE_SET_PIXEL_SHADER_CONSTANT(userMapSamplerId, "UserMapSampler", &TexVar, 1);
		}

		if(defaultVertexShader)
		{
			const irr::core::dimension2du currentRTTSize = services->getVideoDriver()->getCurrentRenderTargetSize();
			const irr::f32 screenX = (irr::f32)currentRTTSize.Width, screenY = (irr::f32)currentRTTSize.Height;

			XE_SET_VERTEX_SHADER_CONSTANT(screenXId, "screenX", &screenX, 1);
			XE_SET_VERTEX_SHADER_CONSTANT(screenYId, "screenY", &screenY, 1);

			irr::scene::ISceneManager* smgr = effect->getActiveSceneManager();
			irr::scene::ICameraSceneNode* cam = smgr->getActiveCamera();

			const irr::core::position2di tLeft = services->getVideoDriver()->getViewPort().UpperLeftCorner;
			const irr::core::position2di bRight = services->getVideoDriver()->getViewPort().LowerRightCorner;

			const irr::core::line3df sLines[4] =
			{
				smgr->getSceneCollisionManager()->getRayFromScreenCoordinates
				(irr::core::position2di(tLeft.X, tLeft.Y), cam),
				smgr->getSceneCollisionManager()->getRayFromScreenCoordinates
				(irr::core::position2di(bRight.X, tLeft.Y), cam),
				smgr->getSceneCollisionManager()->getRayFromScreenCoordinates
				(irr::core::position2di(tLeft.X, bRight.Y), cam),
				smgr->getSceneCollisionManager()->getRayFromScreenCoordinates
				(irr::core::position2di(bRight.X, bRight.Y), cam)
			};

			XE_SET_VERTEX_SHADER_CONSTANT(lineStarts0Id, "LineStarts0", &sLines[0].start.X, 3);
			XE_SET_VERTEX_SHADER_CONSTANT(lineStarts1Id, "LineStarts1", &sLines[1].start.X, 3);
			XE_SET_VERTEX_SHADER_CONSTANT(lineStarts2Id, "LineStarts2", &sLines[2].start.X, 3);
			XE_SET_VERTEX_SHADER_CONSTANT(lineStarts3Id, "LineStarts3", &sLines[3].start.X, 3);

			XE_SET_VERTEX_SHADER_CONSTANT(lineEnds0Id, "LineEnds0", &sLines[0].end.X, 3);
			XE_SET_VERTEX_SHADER_CONSTANT(lineEnds1Id, "LineEnds1", &sLines[1].end.X, 3);
			XE_SET_VERTEX_SHADER_CONSTANT(lineEnds2Id, "LineEnds2", &sLines[2].end.X, 3);
			XE_SET_VERTEX_SHADER_CONSTANT(lineEnds3Id, "LineEnds3", &sLines[3].end.X, 3);
		}

		if(uniformDescriptors.size())
		{
			irr::core::map<irr::core::stringc, SUniformDescriptor>::Iterator mapIter = uniformDescriptors.getIterator();

			for(;!mapIter.atEnd();mapIter++)
			{
				if(mapIter.getNode()->getValue().fPointer == 0)
					continue;

				XE_SET_PIXEL_SHADER_CONSTANT(mapIter.getNode()->getValue().id, mapIter.getNode()->getKey().c_str(), mapIter.getNode()->getValue().fPointer,
					mapIter.getNode()->getValue().paramCount);
			}
		}
	}

	struct SUniformDescriptor
	{
		SUniformDescriptor() : fPointer(0), paramCount(0) {}

		SUniformDescriptor(const irr::f32* fPointerIn, irr::u32 paramCountIn)
			: fPointer(fPointerIn), paramCount(paramCountIn) 
			, id(-2)
		{}

		const irr::f32* fPointer;
		irr::u32 paramCount;
		irr::s32 id;
	};

	irr::core::map<irr::core::stringc, SUniformDescriptor> uniformDescriptors;

private:
	EffectHandler* effect;
	bool defaultVertexShader;

	irr::s32 colorMapSamplerId;
	irr::s32 screenMapSamplerId;
	irr::s32 depthMapSamplerId;
	irr::s32 userMapSamplerId;
	irr::s32 screenXId;
	irr::s32 screenYId;
	irr::s32 lineStarts0Id;
	irr::s32 lineStarts1Id;
	irr::s32 lineStarts2Id;
	irr::s32 lineStarts3Id;
	irr::s32 lineEnds0Id;
	irr::s32 lineEnds1Id;
	irr::s32 lineEnds2Id;
	irr::s32 lineEnds3Id;
};

#undef XE_SET_VERTEX_SHADER_CONSTANT
#undef XE_SET_PIXEL_SHADER_CONSTANT

#endif
