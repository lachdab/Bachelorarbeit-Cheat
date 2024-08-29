#pragma once
#include <opencv2/opencv.hpp>
#include <d3d11.h>
#include <string>
#include <vector>

#ifndef AI_H
#define AI_H

namespace YOLO 
{
    struct Detection 
    {
        int class_id = 0;
        std::string className;
        float confidence = 0.0f;
        cv::Scalar color;
        cv::Rect box;
    };

    class Inference 
    {
    public:
        Inference(const std::string& onnxModelPath, const cv::Size& modelInputShape = { 640, 640 }, bool runWithCuda = false);
        std::vector<Detection> runInference(const cv::Mat& input);
        float modelScoreThreshold = 0.45f;
        float modelNMSThreshold = 0.50f;
    private:
        void loadOnnxNetwork();
        cv::Mat formatToSquare(const cv::Mat& source);

        std::string modelPath;
        bool cudaEnabled;
        cv::Size2f modelShape;
        std::vector<std::string> classes{ "Chest", "Head", "Leg", "Player"};
        cv::dnn::Net net;
    };
    extern void DrawBoundingBoxes(const std::vector<YOLO::Detection>& detections, const std::vector<std::string>& classesToShow);
}

#endif // AI_H

// Erweiterungen der KI
namespace ExtendedAI
{
    struct Target 
    {
        std::string className;
        cv::Rect box;
        float confidence;
    };

    extern float humanErrorChance;
    extern float perfectAimChance;
    extern float maxHumanError;
    extern float aimSpeed;
    extern float screenSizeX;
    extern float screenSizeY;

    extern std::vector<YOLO::Detection> PrioritizeBodyParts(const std::vector<YOLO::Detection>& detections, const std::vector<std::string>& classesToShow);
    extern cv::Point2f SimulateHumanAiming(const cv::Point2f& currentAim, const cv::Point2f& targetAim, float speed);
    extern cv::Point2f AddHumanError(const cv::Point2f& aimPoint, float maxError);
}