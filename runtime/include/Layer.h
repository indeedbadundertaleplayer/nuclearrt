#pragma once

#include <string>
#include <vector>
#include "ShaderFactory.h"
class ObjectInstance;

class Layer {
public:
	Layer(std::string name, float XCoefficient, float YCoefficient)
		: Name(name)
		, XCoefficient(XCoefficient)
		, YCoefficient(YCoefficient)
	{
	}

	std::string Name;
	std::unique_ptr<Shader> layerShader = std::make_unique<None>();
	void SetShader(const std::string& newShader) {
		auto it = shaderFactory.find(newShader);
		if (it == shaderFactory.end()) return;
		layerShader = shaderFactory.at(newShader)();
		std::cout << "Shader Code Size : " << layerShader->codeSize << "\n";
		Application::Instance().GetBackend()->CreateShader(newShader, layerShader->numSamplers, layerShader->numUniformBuffers, layerShader->code, layerShader->codeSize);
	}
	void SetShaderParam(std::string name, float value) {
		layerShader->SetParameter(name, value);
		Application::Instance().GetBackend()->SetFragmentUniforms(name, 0, layerShader->GetData(), sizeof(layerShader->GetData()));
	}
	float GetShaderParam(std::string name) {return layerShader->GetParameter(name);}
	float XCoefficient;
	float YCoefficient;

	std::vector<ObjectInstance*> instances;
};
