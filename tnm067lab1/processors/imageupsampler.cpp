#include <inviwo/core/util/logcentral.h>
#include <modules/opengl/texture/textureutils.h>
#include <modules/tnm067lab1/processors/imageupsampler.h>
#include <modules/tnm067lab1/utils/interpolationmethods.h>
#include <inviwo/core/datastructures/image/layerram.h>
#include <inviwo/core/datastructures/image/layerramprecision.h>
#include <inviwo/core/util/imageramutils.h>

namespace inviwo {

namespace detail {

template <typename T>
void upsample(ImageUpsampler::IntepolationMethod method, const LayerRAMPrecision<T>& inputImage,
              LayerRAMPrecision<T>& outputImage) {
    using F = typename float_type<T>::type;

    const size2_t inputSize = inputImage.getDimensions();
    const size2_t outputSize = outputImage.getDimensions();

    const T* inPixels = inputImage.getDataTyped();
    T* outPixels = outputImage.getDataTyped();

    auto inIndex = [&inputSize](auto pos) -> size_t {
        pos = glm::clamp(pos, decltype(pos)(0), decltype(pos)(inputSize - size2_t(1)));
        return pos.x + pos.y * inputSize.x;
    };
    auto outIndex = [&outputSize](auto pos) -> size_t {
        pos = glm::clamp(pos, decltype(pos)(0), decltype(pos)(outputSize - size2_t(1)));
        return pos.x + pos.y * outputSize.x;
    };

    util::forEachPixel(outputImage, [&](ivec2 outImageCoords) {
        // outImageCoords: Exact pixel coordinates in the output image currently writing to
        // inImageCoords: Relative coordinates of outImageCoords in the input image, might be
        // between pixels
        dvec2 inImageCoords =
            ImageUpsampler::convertCoordinate(outImageCoords, inputSize, outputSize);

        T finalColor(0);

        // DUMMY COLOR, remove or overwrite this bellow
        //finalColor = inPixels[inIndex(glm::clamp(size2_t(outImageCoords), size2_t(0), size2_t(inputSize - size2_t(1))))];

        switch (method) {
            case ImageUpsampler::IntepolationMethod::PiecewiseConstant: {
                // Task 6, Piecewise
                // inPixels contains the color of each pixel in inImage

                //Find imagecoord and round them off to integers
                //Calc with inIndex the pixel index from the rounded off imagecoord
                //With the pixel index determinate the finalcolor.
                finalColor = inPixels[inIndex(round(inImageCoords))];

                break;
            }
            case ImageUpsampler::IntepolationMethod::Bilinear: {
                //Task 7, Bilinear
                //
                ivec2 int_pos = floor(inImageCoords);

                std::array<T, 4> edges = {
                    inPixels[inIndex(int_pos)],                // Top left (2x2)
                    inPixels[inIndex(int_pos + ivec2(1, 0))],  // Top right
                    inPixels[inIndex(int_pos + ivec2(0, 1))],  // bottom left
                    inPixels[inIndex(int_pos + ivec2(1, 1))],  // bottom right
                };

                double x = inImageCoords.x - int_pos.x;
                double y = inImageCoords.y - int_pos.y;

                finalColor = TNM067::Interpolation::bilinear(edges, x, y);

                break;
            }
            case ImageUpsampler::IntepolationMethod::Biquadratic: {

                // Task 8 Biquadric interpolation

                ivec2 int_pos = floor(inImageCoords);

                std::array<T, 9> support_points = {
                    inPixels[inIndex(int_pos)],                // bottom left
                    inPixels[inIndex(int_pos + ivec2(1, 0))],  // bottom center
                    inPixels[inIndex(int_pos + ivec2(2, 0))],  // bottom right
                    inPixels[inIndex(int_pos + ivec2(0, 1))],  // center left
                    inPixels[inIndex(int_pos + ivec2(1, 1))],  // center center
                    inPixels[inIndex(int_pos + ivec2(2, 1))],  // center right
                    inPixels[inIndex(int_pos + ivec2(0, 2))],  // top left
                    inPixels[inIndex(int_pos + ivec2(1, 2))],  // top center
                    inPixels[inIndex(int_pos + ivec2(2, 2))],  // Top right
                };

                double x = (inImageCoords.x - int_pos.x) / 2.0;
                double y = (inImageCoords.y - int_pos.y) / 2.0;

                finalColor = TNM067::Interpolation::biQuadratic(support_points, x, y);

                break;
            }
            case ImageUpsampler::IntepolationMethod::Barycentric: {

                // Task 9 - Barycentric

                ivec2 int_pos = floor(inImageCoords);

                std::array<T, 4> edges = {
                    inPixels[inIndex(int_pos)],                // Top left (2x2)
                    inPixels[inIndex(int_pos + ivec2(1, 0))],  // Top right
                    inPixels[inIndex(int_pos + ivec2(0, 1))],  // bottom left
                    inPixels[inIndex(int_pos + ivec2(1, 1))],  // bottom right
                };

                double x = inImageCoords.x - int_pos.x;
                double y = inImageCoords.y - int_pos.y;

                finalColor = TNM067::Interpolation::barycentric(edges, x, y);
                break;
            }
            default:
                break;
        }

        outPixels[outIndex(outImageCoords)] = finalColor;
    });
}

}  // namespace detail

const ProcessorInfo ImageUpsampler::processorInfo_{
    "org.inviwo.imageupsampler",  // Class identifier
    "Image Upsampler",            // Display name
    "TNM067",                     // Category
    CodeState::Experimental,      // Code state
    Tags::None,                   // Tags
};
const ProcessorInfo ImageUpsampler::getProcessorInfo() const { return processorInfo_; }

ImageUpsampler::ImageUpsampler()
    : Processor()
    , inport_("inport", true)
    , outport_("outport", true)
    , interpolationMethod_("interpolationMethod", "Interpolation Method",
                           {
                               {"piecewiseconstant", "Piecewise Constant (Nearest Neighbor)",
                                IntepolationMethod::PiecewiseConstant},
                               {"bilinear", "Bilinear", IntepolationMethod::Bilinear},
                               {"biquadratic", "Biquadratic", IntepolationMethod::Biquadratic},
                               {"barycentric", "Barycentric", IntepolationMethod::Barycentric},
                           }) {
    addPort(inport_);
    addPort(outport_);
    addProperty(interpolationMethod_);
}

void ImageUpsampler::process() {
    auto inputImage = inport_.getData();
    if (inputImage->getDataFormat()->getComponents() != 1) {
        LogError("The ImageUpsampler processor does only support single channel images");
    }

    auto inSize = inport_.getData()->getDimensions();
    auto outDim = outport_.getDimensions();

    auto outputImage = std::make_shared<Image>(outDim, inputImage->getDataFormat());
    outputImage->getColorLayer()->setSwizzleMask(inputImage->getColorLayer()->getSwizzleMask());
    outputImage->getColorLayer()
        ->getEditableRepresentation<LayerRAM>()
        ->dispatch<void, dispatching::filter::Scalars>([&](auto outRep) {
            auto inRep = inputImage->getColorLayer()->getRepresentation<LayerRAM>();
            detail::upsample(interpolationMethod_.get(), *(const decltype(outRep))(inRep), *outRep);
        });

    outport_.setData(outputImage);
}

dvec2 ImageUpsampler::convertCoordinate(ivec2 outImageCoords, size2_t inputSize, size2_t outputSize) {
    // TODO implement
    dvec2 c(outImageCoords);

    // TASK 5: Convert the outImageCoords to its coordinates in the input image
    dvec2 factor = dvec2(inputSize) / dvec2(outputSize);

    return (c * factor);
}

}  // namespace inviwo
