#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "ObjectGlobalData.h"
#include "ShaderFactory.h"
class ObjectInstance {
public:
    ObjectInstance(unsigned int objectInfoHandle, int type, std::string name)
        : ObjectInfoHandle(objectInfoHandle), Type(type), Name(name) {}
    virtual ~ObjectInstance() = default;
	
	unsigned int Handle = 0;
	unsigned int Type = 0;
	unsigned int ObjectInfoHandle = 0;
	std::string Name = "";
	
	int X = 0;
	int Y = 0;
	float scaleX = 1.0f;
	float scaleY = 1.0f;
	unsigned int Layer = 0;
	std::unique_ptr<Shader> shader = std::make_unique<None>();
	short InstanceValue = 0;
	std::vector<short> Qualifiers = { -1, -1, -1, -1, -1, -1, -1, -1 };

	bool global = false;

	int RGBCoefficient = 0xFFFFFF;
	int Effect = 0;
	unsigned char GetEffectParameter() const {
		return EffectParameter;
	}
	void SetEffectParameter(int effectParameter) {
		EffectParameter = static_cast<unsigned char>(std::clamp(effectParameter, 0, 255));
	}

	void SetShader(const std::string& newShader) {
		auto it = shaderFactory.find(newShader);
		if (it == shaderFactory.end()) return;
		shader = shaderFactory.at(newShader)();
		std::cout << "Shader Code Size : " << shader->codeSize << "\n";
		Application::Instance().GetBackend()->CreateShader(newShader, shader->numSamplers, shader->numUniformBuffers, shader->code, shader->codeSize);
	}
	
	void SetShaderParam(std::string name, float value) const {
		shader->SetParameter(name, value);
		Application::Instance().GetBackend()->SetFragmentUniforms(shader->Name, 0, shader->GetData(), sizeof(shader->GetData()));
	}

	float GetShaderParam(std::string name) const {return shader->GetParameter(name);}
	unsigned int GetAngle() const {
		return Angle;
	}

	void SetAngle(int angle) {
		//limit to 0-359 degrees
		while (angle < 0) {
			angle += 360;
		}
		while (angle >= 360) {
			angle -= 360;
		}
		
		Angle = angle;
	}

	virtual ObjectGlobalData* CreateGlobalData() { return nullptr; };
	virtual void ApplyGlobalData(ObjectGlobalData* globalData) { };

	virtual std::vector<unsigned int> GetImagesUsed() { return std::vector<unsigned int>(); };
	virtual std::vector<unsigned int> GetFontsUsed() { return std::vector<unsigned int>(); };

private:
	unsigned int Angle = 0;
	unsigned char EffectParameter = 0;
};
