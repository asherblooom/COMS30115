#ifndef OBJ_READER_HPP
#define OBJ_READER_HPP

#include <ModelTriangle.h>
#include <string>
#include <vector>

std::vector<ModelTriangle> readObjFile(std::string objFile, std::string mtlFile, float scale);

#endif
