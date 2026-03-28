#pragma once
#include "Application.h"
#include "Extension.h"
#include "ObjectInstance.h"
#include <string>
class ArrayExtension : public Extension {
public:
    ArrayExtension(unsigned int objectInfoHandle, int type, std::string name, short xDimension, short yDimension, short zDimension, short arrType, short flags) : Extension(objectInfoHandle, type, name), Xdimension(xDimension), Ydimension(yDimension), Zdimension(zDimension), Type(arrType), Flags(flags) {}
    void Initialize() override;
private:
    short Xdimension = 10;
    short Ydimension = 10;
    short Zdimension = 1;
    int Xindex, Yindex, Zindex;
    short Type = 0;
    short Flags = 0;
};