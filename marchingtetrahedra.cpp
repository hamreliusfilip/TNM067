#include <inviwo/tnm067lab3/processors/marchingtetrahedra.h>
#include <inviwo/core/datastructures/geometry/basicmesh.h>
#include <inviwo/core/datastructures/volume/volumeram.h>
#include <inviwo/core/util/indexmapper.h>
#include <inviwo/core/util/assertion.h>
#include <inviwo/core/network/networklock.h>
#include <modules/tnm067lab1/utils/interpolationmethods.h>
#include <iostream>
#include <fstream>

namespace inviwo {

size_t MarchingTetrahedra::HashFunc::max = 1;

const ProcessorInfo MarchingTetrahedra::processorInfo_{
    "org.inviwo.MarchingTetrahedra",  // Class identifier
    "Marching Tetrahedra",            // Display name
    "TNM067",                         // Category
    CodeState::Stable,                // Code state
    Tags::None,                       // Tags
};
const ProcessorInfo MarchingTetrahedra::getProcessorInfo() const { return processorInfo_; }

MarchingTetrahedra::MarchingTetrahedra()
: Processor()
, volume_("volume")
, mesh_("mesh")
, isoValue_("isoValue", "ISO value", 0.5f, 0.0f, 1.0f) {
    
    addPort(volume_);
    addPort(mesh_);
    
    addProperty(isoValue_);
    
    isoValue_.setSerializationMode(PropertySerializationMode::All);
    
    volume_.onChange([&]() {
        if (!volume_.hasData()) {
            return;
        }
        NetworkLock lock(getNetwork());
        float iso = (isoValue_.get() - isoValue_.getMinValue()) /
        (isoValue_.getMaxValue() - isoValue_.getMinValue());
        const auto vr = volume_.getData()->dataMap_.valueRange;
        isoValue_.setMinValue(static_cast<float>(vr.x));
        isoValue_.setMaxValue(static_cast<float>(vr.y));
        isoValue_.setIncrement(static_cast<float>(glm::abs(vr.y - vr.x) / 50.0));
        isoValue_.set(static_cast<float>(iso * (vr.y - vr.x) + vr.x));
        isoValue_.setCurrentStateAsDefault();
    });
}

void MarchingTetrahedra::process() {
    auto volume = volume_.getData()->getRepresentation<VolumeRAM>();
    MeshHelper mesh(volume_.getData());
    
    const auto& dims = volume->getDimensions();
    MarchingTetrahedra::HashFunc::max = dims.x * dims.y * dims.z;
    
    const float iso = isoValue_.get();
    
    util::IndexMapper3D mapVolPosToIndex(dims);
    
    const static size_t tetrahedraIds[6][4] = {{0, 1, 2, 5}, {1, 3, 2, 5}, {3, 2, 5, 7},
        {0, 2, 4, 5}, {6, 4, 2, 5}, {6, 7, 5, 2}};
    
    size3_t pos{};
    for (pos.z = 0; pos.z < dims.z - 1; ++pos.z) {
        for (pos.y = 0; pos.y < dims.y - 1; ++pos.y) {
            for (pos.x = 0; pos.x < dims.x - 1; ++pos.x) {
                
                // The DataPoint index should be the 1D-index for the DataPoint in the cell
                // Use volume->getAsDouble to query values from the volume
                // Spatial position should be between 0 and 1
                
                /* --------------------------- TASK 2 ----------------------------- */
                
                /*
                 
                 Create a nested for loop to loop over the 8 data points needed to construct the cell.
                 
                 Set the spatial position, function value and 1D-index of each data point.
                 
                 Use the functions implemented in the previous step to ensure you access the cell
                 with the correct index and use the scaled spatial position:
                 
                 vec3 MarchingTetrahedra::calculateDataPointPos(size3_t posVolume, ivec3 posCell, ivec3 dims)
                 
                 Also, note that the index used to access the cell is different from the index of the data point.
                 */
                
                
                // Iterate through the 8 indexes of the cubes
                
                Cell c; // Our cell to be created.
                size_t index = 0;
                
                for(int x = 0; x < 2; x++){
                    for(int y = 0; y < 2; y++){
                        for(int z; z < 2; z++){
                            
                            // 2^3 = 8 points iterated. All points constructing the cube.
                            
                            vec3 scaledCellPos = calculateDataPointPos(pos, cellPos, dims);
                            size_t cellIndex = calculateDataPointIndexInCell(cellPos);
                            float value = volume->getAsDouble(vec3{pos.x + x, pos.y + y, pos.z + z});
                            
                            //c.dataPoints[cellIndex] = MarchingTetrahedra::DataPoint{scaledCellPos, value, cellIndex};
                            
                            c.dataPoints[index].pos = scaledCellPos;
                            c.dataPoints[index].value = value;
                            c.dataPoints[index].index = cellIndex;
                            
                            index++;
                        }
                    }
                }
                
                
                /* -------------------------- TASK 3 ----------------------------- */
                
                // TODO: TASK 3: Subdivide cell into 6 tetrahedra (hint: use tetrahedraIds)
                
                std::vector<Tetrahedra> tetrahedras;
                
                // Temp tetrahedra
                Tetrahedra tetra;
                
                // Loop trough the six diffrent tetrahedras, 6 tetrahedras per cell (Box).
                for (size_t i = 0; i < 6; i++) {
                    
                    // Go though all 4 indexies for each tetrahedra and assign the correct cell index, value and position.
                    for (size_t j = 0; j < 4; j++) {
                        tetra.dataPoints[j] = c.dataPoints[tetrahedraIds[i][j]];
                    }
                    
                    // Save all 6 tetrahedra in the vector (the vector will contain 6 * 8 tetrahedras)
                    tetrahedras.push_back(tetra);
                }
                
                
                /* -------------------------- TASK 4 ----------------------------- */
                
                // Calcualte the case ID for each tetrahedra, will give 0,1 or 2 triangles depending of wich case.
                
                for (const Tetrahedra& tetrahedra : tetrahedras) {
                    // Step three: Calculate for tetra case index
                    int caseId = 0;
                    
                    for (size_t i = 0; i < 4; ++i)
                        if (tetrahedra.dataPoints[i].value > iso)
                            caseId |= (int) pow(2, i); // pow(2, i) = 1, 2, 4 or 8
                    
                    /*
                     
                     pow(2, i) calculates 2 raised to the power of i. Since i ranges from 0 to 3
                     in the loop (for (size_t i = 0; i < 4; ++i)), it evaluates to 1, 2, 4, or 8, respectively.
                     
                     If pow(2, i) evaluates to 1, it sets the least significant bit (LSB) of caseId to 1.
                     If pow(2, i) evaluates to 2, it sets the second least significant bit to 1.
                     If pow(2, i) evaluates to 4, it sets the third least significant bit to 1.
                     If pow(2, i) evaluates to 8, it sets the fourth least significant bit (the leftmost bit in a 4-bit representation) to 1.
                    
                     */
                    
                    // step four: Extract triangles
                    TriangleCreator tc{mesh, iso, tetrahedra};
                    
                    switch (caseId) {
                        case 0:
                        case 15: {
                            break;
                        }
                        case 1:
                        case 14: {
                            tc.createTriangle(caseId == 14, {0, 1}, {0, 3}, {0, 2});
                            break;
                        }
                        case 2:
                        case 13: {
                            tc.createTriangle(caseId == 13, {1, 0}, {1, 2}, {1, 3});
                            break;
                        }
                        case 3:
                        case 12: {
                            tc.createTriangle(caseId == 12, {1, 2}, {1, 3}, {0, 3});
                            tc.createTriangle(caseId == 12, {1, 2}, {0, 3}, {0, 2});
                            break;
                        }
                        case 4:
                        case 11: {
                            tc.createTriangle(caseId == 11, {2, 3}, {2, 1}, {2, 0});
                            break;
                        }
                        case 5:
                        case 10: {
                            tc.createTriangle(caseId == 10, {2, 1}, {0, 1}, {0, 3});
                            tc.createTriangle(caseId == 10, {2, 3}, {2, 1}, {0, 3});
                            break;
                        }
                        case 6:
                        case 9: {
                            tc.createTriangle(caseId == 9, {2, 0}, {1, 3}, {1, 0});
                            tc.createTriangle(caseId == 9, {2, 0}, {1, 3}, {2, 3});
                            break;
                        }
                        case 7:
                        case 8: {
                            tc.createTriangle(caseId == 8, {3, 1}, {3, 0}, {3, 2});
                            break;
                        }
                    }
                }
            }
        }
    }
    
    mesh_.setData(mesh.toBasicMesh());
}

/* ----------------------- TASK 1 ----------------------------- */

int MarchingTetrahedra::calculateDataPointIndexInCell(ivec3 index3D) {
    
    // TODO: TASK 1: map 3D index to 1D
    
    /*The first function maps a 3D index to a 1D index,
     which is then used to access the correct data point within the cell.*/
    
    // Mappar 3d indexet till ett 1d index men behåller ordninge, ental, tiotal, hundratal?
    return ivec3[0] * 1 + ivec[1] * 10 + ivec[2] * 100;
}

vec3 MarchingTetrahedra::calculateDataPointPos(size3_t posVolume, ivec3 posCell, ivec3 dims) {
    
    // TODO: TASK 1: scale DataPoint position with dimensions to be between 0 and 1
    
    /* The function should take the position of the cell in the volume,
     the 3D index of the data point within the cell, and the dimension of the volume.
     This function should return the position of the data point within the volume scaled between 0 and 1.*/
    
    // posVolume: the cell position in the volume
    // posCell: the position within the cell
    // dims: the dimension of the volume
    
    float x = (float(posVolume[0] + posCell[0]) ) / float(dims[0] - 1);
    float y = (float(posVolume[1] + posCell[1]) ) / float(dims[1] - 1);
    float z = (float(posVolume[2] + posCell[2]) ) / float(dims[2] - 1);
    
    // return: the position of the data point within the volume (scaled between 0, 1)
    
    return vec3(x,y,z);
}

MarchingTetrahedra::MeshHelper::MeshHelper(std::shared_ptr<const Volume> vol)
: edgeToVertex_()
, vertices_()
, mesh_(std::make_shared<BasicMesh>())
, indexBuffer_(mesh_->addIndexBuffer(DrawType::Triangles, ConnectivityType::None)) {
    mesh_->setModelMatrix(vol->getModelMatrix());
    mesh_->setWorldMatrix(vol->getWorldMatrix());
}

void MarchingTetrahedra::MeshHelper::addTriangle(size_t i0, size_t i1, size_t i2) {
    IVW_ASSERT(i0 != i1, "i0 and i1 should not be the same value");
    IVW_ASSERT(i0 != i2, "i0 and i2 should not be the same value");
    IVW_ASSERT(i1 != i2, "i1 and i2 should not be the same value");
    
    indexBuffer_->add(static_cast<glm::uint32_t>(i0));
    indexBuffer_->add(static_cast<glm::uint32_t>(i1));
    indexBuffer_->add(static_cast<glm::uint32_t>(i2));
    
    const auto a = std::get<0>(vertices_[i0]);
    const auto b = std::get<0>(vertices_[i1]);
    const auto c = std::get<0>(vertices_[i2]);
    
    const vec3 n = glm::normalize(glm::cross(b - a, c - a));
    std::get<1>(vertices_[i0]) += n;
    std::get<1>(vertices_[i1]) += n;
    std::get<1>(vertices_[i2]) += n;
}

std::shared_ptr<BasicMesh> MarchingTetrahedra::MeshHelper::toBasicMesh() {
    for (auto& vertex : vertices_) {
        // Normalize the normal of the vertex
        std::get<1>(vertex) = glm::normalize(std::get<1>(vertex));
    }
    mesh_->addVertices(vertices_);
    return mesh_;
}

std::uint32_t MarchingTetrahedra::MeshHelper::addVertex(vec3 pos, size_t i, size_t j) {
    IVW_ASSERT(i != j, "i and j should not be the same value");
    if (j < i) std::swap(i, j);
    
    auto [edgeIt, inserted] = edgeToVertex_.try_emplace(std::make_pair(i, j), vertices_.size());
    if (inserted) {
        vertices_.push_back({pos, vec3(0, 0, 0), pos, vec4(0.7f, 0.7f, 0.7f, 1.0f)});
    }
    return static_cast<std::uint32_t>(edgeIt->second);
}

}  // namespace inviwo
