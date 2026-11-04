#include "ObjImporter.h"

#include "Mesh.h"
#include <fstream>
#include <sstream>

vector<Vertex> GetVerticesFromObj(vector<vector<string>>& objData) {
	vector<Vertex> vertices;
	
	for (auto& line : objData) {
		if (line.empty()) continue;
		if (line[0] == "v") {
			double x = stod(line[1]);
			double y = stod(line[2]);
			double z = stod(line[3]);

			double r = x + 1.0 * 0.5f;
			double g = y + 1.0 * 0.5f;
			double b = z + 1.0 * 0.5f;

			vertices.push_back({ vec3(x, y, z), vec3(r, g, b) });
		}
	}
	return vertices;
}

vector<GLuint> GetIndicesFromObj(vector<vector<string>>& objData) {
	vector<GLuint> indices;

	for (auto& line : objData) {
		if (line.empty()) continue;
		if (line[0] == "f") {
			//삼각형 페이스
			if (line.size() == 3 + 1) {
				int idx1 = line[1].find('/') != string::npos ? stoi(line[1].substr(0, line[1].find('/'))) - 1 : stoi(line[1]) - 1;
				int idx2 = line[2].find('/') != string::npos ? stoi(line[2].substr(0, line[2].find('/'))) - 1 : stoi(line[2]) - 1;
				int idx3 = line[3].find('/') != string::npos ? stoi(line[3].substr(0, line[3].find('/'))) - 1 : stoi(line[3]) - 1;

				indices.push_back(static_cast<GLuint>(idx1));
				indices.push_back(static_cast<GLuint>(idx2));
				indices.push_back(static_cast<GLuint>(idx3));
			}
			//사각형 페이스
			else if (line.size() == 4 + 1) {
				int idx1 = line[1].find('/') != string::npos ? stoi(line[1].substr(0, line[1].find('/'))) - 1 : stoi(line[1]) - 1;
				int idx2 = line[2].find('/') != string::npos ? stoi(line[2].substr(0, line[2].find('/'))) - 1 : stoi(line[2]) - 1;
				int idx3 = line[3].find('/') != string::npos ? stoi(line[3].substr(0, line[3].find('/'))) - 1 : stoi(line[3]) - 1;
				int idx4 = line[4].find('/') != string::npos ? stoi(line[4].substr(0, line[4].find('/'))) - 1 : stoi(line[4]) - 1;

				indices.push_back(static_cast<GLuint>(idx1));
				indices.push_back(static_cast<GLuint>(idx2));
				indices.push_back(static_cast<GLuint>(idx3));
				
				indices.push_back(static_cast<GLuint>(idx1));
				indices.push_back(static_cast<GLuint>(idx3));
				indices.push_back(static_cast<GLuint>(idx4));
			}
		}
	}
	return indices;
}

vector<vec3> GetControlPointsFromObj(vector<vector<string>>& objData) {
	vector<vec3> controlPoints;
	for (auto& line : objData) {
		if (line.empty()) continue;
		if (line[0] == "v") {
			double x = stod(line[1]);
			double y = stod(line[2]);
			double z = stod(line[3]);
			controlPoints.push_back(vec3(x, y, z));
		}
	}
	return controlPoints;
}