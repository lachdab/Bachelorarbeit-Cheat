#include <fstream>
#include <imgui/imgui.h>
#include <windows.h>
#include <d3d11.h>
#include "aimbot.h"
#include <random>
#include "il2cpp.h"
#define _USE_MATH_DEFINES
#include <math.h>

namespace YOLO
{
    void DrawBoundingBoxes(const std::vector<YOLO::Detection>& detections, const std::vector<std::string>& classesToShow) {
        for (const auto& detection : detections)
        {
            if (std::find(classesToShow.begin(), classesToShow.end(), detection.className) == classesToShow.end())
            {
                continue;
            }

            const cv::Rect& box = detection.box;
            const cv::Scalar& color = detection.color;

            // Zeichnen des Rechtecks mit ImGui
            ImGui::GetBackgroundDrawList()->AddRect(
                ImVec2(box.x, box.y),
                ImVec2(box.x + box.width, box.y + box.height),
                IM_COL32(color[2], color[1], color[0], 255), // OpenCV verwendet BGR, ImGui RGB
                0.0f, // Keine Rundung
                0,    // Keine Flags
                2.0f  // Linienbreite
            );

            // Text für die Erkennung
            std::string classString = detection.className + ' ' + std::to_string(detection.confidence).substr(0, 4);
            ImGui::GetBackgroundDrawList()->AddText(
                ImVec2(box.x, box.y - 20),
                IM_COL32(255, 255, 255, 255),
                classString.c_str()
            );
        }
    }
    // https://github.com/ultralytics/ultralytics/blob/main/examples/YOLOv8-CPP-Inference/inference.cpp
    Inference::Inference(const std::string& onnxModelPath, const cv::Size& modelInputShape, bool runWithCuda)
    {
        modelPath = onnxModelPath;
        modelShape = modelInputShape;
        cudaEnabled = runWithCuda;
        loadOnnxNetwork();
    }

    void Inference::loadOnnxNetwork() 
    {
        net = cv::dnn::readNetFromONNX(modelPath);
        if (cudaEnabled) 
        {
            net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        }
        else 
        {
            net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }
    }

    std::vector<Detection> Inference::runInference(const cv::Mat& input) 
    {
        cv::Mat modelInput = formatToSquare(input);
        cv::Mat blob;
        cv::dnn::blobFromImage(modelInput, blob, 1.0 / 255.0, modelShape, cv::Scalar(), true, false);
        net.setInput(blob);

        std::vector<cv::Mat> outputs;
        net.forward(outputs, net.getUnconnectedOutLayersNames());

        int rows = outputs[0].size[2];
        int dimensions = outputs[0].size[1];

        outputs[0] = outputs[0].reshape(1, dimensions);
        cv::transpose(outputs[0], outputs[0]);
        float* data = (float*)outputs[0].data;

        float x_factor = modelInput.cols / modelShape.width;
        float y_factor = modelInput.rows / modelShape.height;

        std::vector<int> class_ids;
        std::vector<float> confidences;
        std::vector<cv::Rect> boxes;

        for (int i = 0; i < rows; ++i) 
        {
            float* classes_scores = data + 4;
            
            cv::Mat scores(1, classes.size(), CV_32FC1, classes_scores);
            cv::Point class_id;
            double maxClassScore;
            
            minMaxLoc(scores, 0, &maxClassScore, 0, &class_id);
            
            if (maxClassScore > YOLO::Inference::modelScoreThreshold) 
            {
                confidences.push_back(maxClassScore);
                class_ids.push_back(class_id.x);
                
                float x = data[0];
                float y = data[1];
                float w = data[2];
                float h = data[3];
                
                int left = int((x - 0.5 * w) * x_factor);
                int top = int((y - 0.5 * h) * y_factor);
                
                int width = int(w * x_factor);
                int height = int(h * y_factor);
                boxes.push_back(cv::Rect(left, top, width, height));
            }
            data += dimensions;
        }

        std::vector<int> nms_result;
        cv::dnn::NMSBoxes(boxes, confidences, YOLO::Inference::modelScoreThreshold, YOLO::Inference::modelNMSThreshold, nms_result);

        std::vector<Detection> detections;
        for (unsigned long i = 0; i < nms_result.size(); ++i) 
        {
            int idx = nms_result[i];

            Detection result;
            result.class_id = class_ids[idx];
            result.confidence = confidences[idx];

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dis(100, 255);
            result.color = cv::Scalar(dis(gen), dis(gen), dis(gen));

            result.className = classes[result.class_id];
            result.box = boxes[idx];

            detections.push_back(result);
        }

        return detections;
    }

    cv::Mat Inference::formatToSquare(const cv::Mat& source) 
    {
        int col = source.cols;
        int row = source.rows;
        int _max = MAX(col, row);
        cv::Mat result = cv::Mat::zeros(_max, _max, CV_8UC3);
        source.copyTo(result(cv::Rect(0, 0, col, row)));
        return result;
    }
}

namespace ExtendedAI
{
    float aimSpeed = 0.2f;
    float humanErrorChance = 0.95f;
    float perfectAimChance = 0.05f;
    float maxHumanError = 5.0f;
    float screenSizeX = 0;
    float screenSizeY = 0;
    
    std::vector<YOLO::Detection> PrioritizeBodyParts(const std::vector<YOLO::Detection>& detections, const std::vector<std::string>& classesToShow) 
    {
        std::vector<YOLO::Detection> prioritized;

        auto shouldShowClass = [&classesToShow](const std::string& className) 
            {
                return std::find(classesToShow.begin(), classesToShow.end(), className) != classesToShow.end();
            };

        // Priorisiere Kopf mit niedrigerem Schwellenwert
        if (shouldShowClass("Head"))
        {
            auto head = std::find_if(detections.begin(), detections.end(),
                [](const YOLO::Detection& d)
                {
                    return d.className == "Head" && d.confidence >= 0.3f;
                });
            if (head != detections.end()) prioritized.push_back(*head);
        }

        // Dann Brust
        if (shouldShowClass("Chest")) 
        {
            auto chest = std::find_if(detections.begin(), detections.end(),
                [](const YOLO::Detection& d) 
                {
                    return d.className == "Chest" && d.confidence >= 0.5f;
                });
            if (chest != detections.end()) prioritized.push_back(*chest);
        }

        // Dann Beine
        if (shouldShowClass("Leg")) 
        {
            for (const auto& d : detections) 
            {
                if (d.className == "Leg" && d.confidence >= 0.5f) 
                {
                    prioritized.push_back(d);
                }
            }
        }

        // Zuletzt Spieler
        if (shouldShowClass("Player")) 
        {
            auto player = std::find_if(detections.begin(), detections.end(),
                [](const YOLO::Detection& d) 
                {
                    return d.className == "Player" && d.confidence >= 0.5f;
                });
            if (player != detections.end()) prioritized.push_back(*player);
        }
        return prioritized;
    }

    cv::Point2f SimulateHumanAiming(const cv::Point2f& currentAim, const cv::Point2f& targetAim, float speed)
    {
        // Berechne die Distanz zum Ziel
        cv::Point2f diff = targetAim - currentAim;
        float distance = cv::norm(diff);

        // Berechne den Smoothness-Faktor basierend auf der Bildschirmgröße
        float maxScreenDimension = std::max(screenSizeX, screenSizeY);
        float smoothnessFactor = 1.0f - (distance / maxScreenDimension);
        smoothnessFactor = std::clamp(smoothnessFactor, 0.1f, 1.0f);  // Begrenzen Sie den Faktor auf einen sinnvollen Bereich

        // Entscheide, ob ein menschlicher Fehler gemacht wird
        bool makeHumanError = (static_cast<float>(rand()) / RAND_MAX) < humanErrorChance;

        cv::Point2f aimPoint;
        if (makeHumanError && !((static_cast<float>(rand()) / RAND_MAX) < perfectAimChance)) 
        {
            // Füge menschlichen Fehler hinzu
            aimPoint = AddHumanError(targetAim, maxHumanError);
        }
        else 
        {
            aimPoint = targetAim; // Perfektes Zielen
        }

        // Maximale Bewegung basierend auf der Zielgeschwindigkeit und Smoothness
        float maxMove = distance * speed * smoothnessFactor;

        // Begrenzen Sie die Bewegung auf die maximale Bewegung
        if (distance > maxMove) 
        {
            return currentAim + (diff * maxMove / distance);
        }
        else 
        {
            return aimPoint;
        }
    }

    cv::Point2f AddHumanError(const cv::Point2f& aimPoint, float maxError) 
    {
        float angle = static_cast<float>(rand()) / RAND_MAX * 2 * M_PI;
        float magnitude = static_cast<float>(rand()) / RAND_MAX * maxError;
        return aimPoint + cv::Point2f(cos(angle) * magnitude, sin(angle) * magnitude);
    }
}
