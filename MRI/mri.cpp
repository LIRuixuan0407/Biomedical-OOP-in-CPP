#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>
#include <numeric>
#include "terminal_graphics.h"
#include "load_pgm.h"

using namespace std;

// 创建二值图像
// 这个函数创建一个二值图像，图像中ROI区域的像素值为1，其他区域的像素值为0
// 参数：图像的宽度、高度、ROI中心的x坐标、ROI中心的y坐标、ROI的半径
vector<vector<int>> createBinaryImage(const string& filename, int x_center, int y_center, int radius) {
    ifstream file(filename); // 打开文件
    if (!file.is_open()) { // 检查文件是否成功打开
        cerr << "Failed to open file: " << filename << endl;
        exit(1);
    }

    string magic; // 存储PGM文件的魔数（文件类型标识）
    int width, height, maxval; // 存储图像的宽度、高度和最大像素值
    file >> magic >> width >> height >> maxval; // 从文件中读取这些值
    
    vector<vector<int>> binaryImage(height, vector<int>(width, 0)); // 初始化二值图像，所有像素值为0

    // 遍历图像的每一行和每一列
    for (int i = y_center - radius; i <= y_center + radius; ++i) {
        for (int j = x_center - radius; j <= x_center + radius; ++j) {
            binaryImage[i][j] = 1; // 如果在ROI内，则将像素值设置为1
        }
    }
    return binaryImage; // 返回二值图像
}

// 读取PGM图像文件中的ROI区域
// 这个函数从PGM图像文件中读取指定的ROI（感兴趣区域）
// 参数：文件名、二值图像
vector<int> readROI(const string& filename, const vector<vector<int>>& binaryImage) {
    ifstream file(filename); // 打开文件
    if (!file.is_open()) { // 检查文件是否成功打开
        cerr << "Failed to open file: " << filename << endl;
        exit(1);
    }

    string magic; // 存储PGM文件的魔数（文件类型标识）
    int width, height, maxval; // 存储图像的宽度、高度和最大像素值
    file >> magic >> width >> height >> maxval; // 从文件中读取这些值
    vector<int> roi; // 存储ROI区域的像素值
    int pixel; // 存储当前像素值
    for (int i = 0; i < height; ++i) { // 遍历图像的每一行
        for (int j = 0; j < width; ++j) { // 遍历图像的每一列
            file >> pixel; // 从文件中读取当前像素值
            if (binaryImage[i][j] == 1) { // 检查当前像素是否在二值图像的ROI区域内
                roi.push_back(pixel); // 如果在ROI内，则将像素值添加到ROI向量中
            }
        }
    }

    file.close(); // 关闭文件
    return roi; // 返回ROI区域的像素值
}


// 找到峰值对比度时间帧
// 这个函数找到信号时间序列中峰值对比度的时间帧
// 参数：信号时间序列
int findPeakContrastTimeframe(const vector<double>& signalTimecourse) {
    if (signalTimecourse.empty()) { // 检查信号时间序列是否为空
        cerr << "Error: Signal timecourse is empty." << endl;
        exit(1);
    }

    int peakFrame = 1; // 初始化峰值时间帧
    double peakIntensity = signalTimecourse[0]; // 初始化峰值强度
    for (size_t i = 1; i < signalTimecourse.size(); ++i) { // 遍历信号时间序列
        if (signalTimecourse[i] > peakIntensity) { // 如果当前强度大于峰值强度
            peakIntensity = signalTimecourse[i]; // 更新峰值强度
            peakFrame = i; // 更新峰值时间帧
        }
    }
    return peakFrame; // 返回峰值时间帧
}

// 计算信号梯度
// 这个函数计算信号时间序列的梯度
// 参数：信号时间序列
vector<double> calculateSignalGradient(const vector<double>& signalTimecourse) {
    vector<double> gradient; // 存储信号梯度
    for (size_t i = 1; i < signalTimecourse.size(); ++i) { // 遍历信号时间序列
        gradient.push_back(signalTimecourse[i] - signalTimecourse[i - 1]); // 计算梯度并添加到梯度向量中
    }
    return gradient; // 返回信号梯度
}

// 找到对比剂到达时间帧
// 这个函数找到信号梯度中对比剂到达的时间帧
// 参数：信号梯度
int findContrastArrivalTimeframe(const vector<double>& gradient) {
    if (gradient.empty()) { // 检查信号梯度是否为空
        cerr << "Error: Gradient is empty." << endl;
        exit(1);
    }

    int arrivalFrame = 0; // 初始化到达时间帧
    double threshold = 10; // 设置梯度阈值
    for (size_t i = 1; i < gradient.size(); ++i) { // 遍历信号梯度
        if (gradient[i] > threshold) { // 如果当前梯度大于阈值
            arrivalFrame = i; // 更新到达时间帧
            break; // 退出循环
        }
    }
    return arrivalFrame; // 返回到达时间帧
}

// 读取对比剂信息
// 这个函数从文件中读取对比剂信息
// 参数：文件名、对比剂名称、对比剂剂量
void readContrastInfo(const string& filename, string& agent, float& dose) {
    ifstream file(filename); // 打开文件
    if (!file.is_open()) { // 检查文件是否成功打开
        cerr << "Failed to open file: " << filename << endl;
        exit(1);
    }

    file >> agent >> dose; // 从文件中读取对比剂名称和剂量
    file.close(); // 关闭文件
}

int main() {
    int x_center = 74, y_center = 90; // 设置ROI中心坐标
    int radius = 5 / 2; // 设置ROI半径
    
    vector<double> signalTimecourse; // 存储信号时间序列
    vector<double> gradient; // 存储信号梯度
    
    // 遍历所有图像文件
    vector<vector<int>> binaryImage;
    for (int i = 1; i <= 20; ++i) {
        string filename = "data/mri-" + (i < 10 ? "0" + to_string(i) : to_string(i)) + ".pgm"; // 构建文件名
        if (i == 1) {
            // 创建二值图像
            binaryImage = createBinaryImage(filename, x_center, y_center, radius);
        }
        vector<int> roi = readROI(filename, binaryImage); // 读取ROI区域的像素值
        double avgSignal = accumulate(roi.begin(), roi.end(), 0.0) / roi.size(); // 计算ROI区域的平均信号强度
        signalTimecourse.push_back(avgSignal); // 将平均信号强度添加到信号时间序列中
    }
    
    int peakFrame = findPeakContrastTimeframe(signalTimecourse); // 找到峰值对比剂时间帧
    cout << "Image at peak contrast concentration:" << endl;
    string peakImage = "data/mri-" + (peakFrame < 10 ? "0" + to_string(peakFrame) : to_string(peakFrame)) + ".pgm"; // 构建峰值图像文件名
    const auto image = load_pgm(peakImage); // 加载峰值图像
    
    TG::imshow(TG::magnify(image, 2), 0, 255); // 显示峰值图像，放大2倍
    cout << endl;
    
    gradient = calculateSignalGradient(signalTimecourse); // 计算信号时间序列的梯度
    int arrivalFrame = findContrastArrivalTimeframe(gradient); // 找到对比剂到达时间帧
    
    cout << "Signal timecourse within ROI:" << endl;
    for (double intensity : signalTimecourse) { // 输出信号时间序列
        cout << intensity << " ";
    }
    cout << endl;
    TG::plot(1000, 300) // 绘制信号时间序列曲线图
        .add_line(signalTimecourse);
    
    cout << "Gradient of signal timecourse within ROI:" << endl;
    for (double grad : gradient) { // 输出信号梯度
        cout << grad << " ";
    }
    cout << endl;
    
    // 绘制信号梯度曲线图
    TG::plot(1000, 300)
        .add_line(gradient, 3);
    
    
    string agent; // 存储对比剂名称
    float dose; // 存储对比剂剂量
    readContrastInfo("data/contrast_info.txt", agent, dose); // 读取对比剂信息
    
    cout << "Contrast agent: " << agent << ", dose: " << dose << endl;
    
    cout << "Contrast arrival occurs at frame " << arrivalFrame << ", with signal intensity:" << signalTimecourse[arrivalFrame] << endl;
    
    cout << "Peak contrast concentration occurs at frame " << peakFrame << ", with signal intensity:" << signalTimecourse[peakFrame] << endl;
    
    cout << "Temporal gradient of signal during contrast uptake: " <<  (signalTimecourse[peakFrame] - signalTimecourse[arrivalFrame]) / (peakFrame - arrivalFrame) << endl;
    
    return 0;
}
