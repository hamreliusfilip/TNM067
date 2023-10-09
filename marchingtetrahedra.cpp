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

                // TODO: TASK 2: create a nested for loop to construct the cell
                Cell c;

                for(int z = 0; z < 2; ++z) {
                    for(int y = 0; y < 2; ++y) {
                        for(int x = 0; x < 2; ++x) {
                            
                            // position for cell
                            ivec3 cell_position = vec3{x, y, z};

                            // position of volume, 0-1
                            vec3 scale_position = calculateDataPointPos(pos, cell_position, dims);

                            // cell position in volume
                            size3_t cellPosInVol(pos.x + x, pos.y + y, pos.z + z);

                            //index value from 3D to 1D
                            int one_dim = calculateDataPointIndexInCell(cell_position);

                            // Query the values from the volume
                            float value = static_cast<float>(volume->getAsDouble(cellPosInVol));

                            size_t index = mapVolPosToIndex(cellPosInVol);

                            c.dataPoints[one_dim] = {scale_position, value, index};

                        }
                    }
                }

                // TODO: TASK 3: Subdivide cell into 6 tetrahedra (hint: use tetrahedraIds)
                std::vector<Tetrahedra> tetrahedras;

                // i: tetrahedra, j: data points
                for (size_t i = 0; i < 6; ++i) {
                    Tetrahedra copy;

                    // 4 data points in one tetrahedra
                    for (int j = 0; j < 4; ++j) {
                        size_t get_Id = tetrahedraIds[i][j]; // [6][4]
                        DataPoint data_point = c.dataPoints[get_Id]; // Get the DataPoint from the cell
                        copy.dataPoints[j] = data_point; //  temp datapoint?
                    }
                    tetrahedras.push_back(copy); // Add tetrahedra to the list
                }


                for (const Tetrahedra& tetrahedra : tetrahedras) {
                    // TODO: TASK 4: Calculate case id for each tetrahedra, and add triangles for

                    int caseId = 0;

                    // > iso gives positive vertex, < iso gives negative vertex
                    // Calculate caseId depending on iso
                    for(int i = 0; i < 4; ++i) {
                        if(tetrahedra.dataPoints[i].value < iso) {
                            caseId += pow(2.0, i);
                        }
                    }
                    
                    // Get the veticies for the tetrahedra (struct datapoint from struct tetrahedra).
                    DataPoint v0 = tetrahedra.dataPoints[0];
                    DataPoint v1 = tetrahedra.dataPoints[1];
                    DataPoint v2 = tetrahedra.dataPoints[2];
                    DataPoint v3 = tetrahedra.dataPoints[3];

                    // Extract triangles
                    switch (caseId) {
                       case 0:
                       case 15:
                           break;
                       case 1:
                       case 14: {

                           if (caseId == 1) {
                               create_tri(v0, v1, v0, v2, v0, v3, iso, mesh);
                           } else {
                               create_tri(v0, v1, v0, v3, v0, v2, iso, mesh);
                           }

                           break;
                       }
                       case 2:
                       case 13: {
                           if (caseId == 2) {
                               create_tri(v1, v0, v1, v3, v1, v2, iso, mesh);
                           } else {
                               create_tri(v1, v0, v1, v2, v1, v3, iso, mesh);
                           }

                           break;
                       }
                       case 3:
                       case 12: {
                           if (caseId == 3) {
                               create_tri(v0, v3, v1, v3, v1, v2, iso, mesh);
                               create_tri(v0, v3, v1, v2, v0, v2, iso, mesh);
                           } else {
                               create_tri(v0, v3, v1, v2, v1, v3, iso, mesh);
                               create_tri(v0, v3, v0, v2, v1, v2, iso, mesh);
                           }
                           
                           break;
                       }
                       case 4:
                       case 11: {
                           if (caseId == 4) {
                               create_tri(v2, v0, v2, v1, v2, v3, iso, mesh);
                           } else {
                               create_tri(v2, v0, v2, v3, v2, v1, iso, mesh);
                           }

                           break;
                       }
                       case 5:
                       case 10: {
                           if (caseId == 5) {
                               create_tri(v0, v3, v0, v1, v2, v1, iso, mesh);
                               create_tri(v0, v3, v2, v1, v2, v3, iso, mesh);
                           } else {
                               create_tri(v0, v3, v2, v1, v0, v1, iso, mesh);
                               create_tri(v0, v3, v2, v3, v2, v1, iso, mesh);
                           }

                           break;
                       }
                       case 6:
                       case 9: {
                           if (caseId == 6) {
                               create_tri(v2, v0, v1, v0, v1, v3, iso, mesh);
                               create_tri(v2, v0, v2, v3, v1, v3, iso, mesh);
                           } else {
                               create_tri(v2, v0, v1, v3, v1, v0, iso, mesh);
                               create_tri(v2, v0, v1, v3, v2, v3, iso, mesh);
                           }

                           break;
                       }
                       case 7:
                       case 8: {
                           if (caseId == 7) {
                               create_tri(v3, v1, v3, v2, v3, v0, iso, mesh);
                           } else {
                               create_tri(v3, v1, v3, v0, v3, v2, iso, mesh);
                           }
                        
                           break;
                        }
                    }
                }
            }
        }
    }

    mesh_.setData(mesh.toBasicMesh());
}


// interpolation for the new datapoints
vec3 MarchingTetrahedra::interpolation(const DataPoint& v_first, const DataPoint& v_second, const float iso) {
    return v_first.pos + (iso - v_first.value) * ((v_second.pos - v_first.pos) / (v_second.value - v_first.value));
}

// all edges, iso, tetra and mesh is needed
void MarchingTetrahedra::create_tri(const DataPoint& v0, const DataPoint& v1, const DataPoint& v2, const DataPoint& v3,
                                    const DataPoint& v4, const DataPoint& v5, const float iso, MeshHelper& mesh) {
    //interpolate to get v1, v2 and v3
    vec3 inter0 = interpolation(v0, v1, iso);
    vec3 inter1 = interpolation(v2, v3, iso);
    vec3 inter2 = interpolation(v4, v5, iso);

    auto new_v0 = mesh.addVertex(inter0, v0.indexInVolume, v1.indexInVolume);
    auto new_v1 = mesh.addVertex(inter1, v2.indexInVolume, v3.indexInVolume);
    auto new_v2 = mesh.addVertex(inter2, v4.indexInVolume, v5.indexInVolume);

    mesh.addTriangle(new_v0, new_v1, new_v2);
}

int MarchingTetrahedra::calculateDataPointIndexInCell(ivec3 index3D) {
    // TODO: TASK 1: map 3D index to 1D
    // Turning coordinates into data point (vertex)
    return 1 * index3D.x + 2 * index3D.y + 4 * index3D.z; // Binary base, 2^n
}

vec3 MarchingTetrahedra::calculateDataPointPos(size3_t posVolume, ivec3 posCell, ivec3 dims) {
    // TODO: TASK 1: scale DataPoint position with dimensions to be between 0 and 1
    
    // Overall position of data point divided with dimension
    return (vec3(posVolume) + vec3(posCell)) / vec3(dims - 1);
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
