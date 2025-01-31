// C++
#include <iostream>
#include <chrono>
#include <string>
#include <io.h>
#include <vector>
#include <direct.h>
// OpenCV
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
// Azure Kinect DK
#include <k4a/k4a.hpp>
#include "slamBase.h"
// PCL 库
#include<pcl/io/io.h>
#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>
#include <pcl/point_types.h>
#include<pcl/visualization/cloud_viewer.h>


// 定义点云类型
typedef pcl::PointXYZRGB PointT;
typedef pcl::PointCloud<PointT> PointCloud;
using namespace cv;
using namespace std;


int main(int argc, char* argv[]) {
	// 发现已连接的设备数
	const uint32_t device_count = k4a::device::get_installed_count();
	if (0 == device_count) {
		std::cout << "Error: no K4A devices found. " << std::endl;
		return -1;
	}
	else {
		std::cout << "Found " << device_count << " connected devices. " <<
			std::endl;
		if (1 != device_count)// 超过 1 个设备，也输出错误信息。
		{
			std::cout << "Error: more than one K4A devices found. " << std::endl;
			return -1;
		}
		else// 该示例代码仅限对 1 个设备操作
		{
			std::cout << "Done: found 1 K4A device. " << std::endl;
		}
	}
	// 打开（默认）设备
	k4a::device device = k4a::device::open(K4A_DEVICE_DEFAULT);
	std::cout << "Done: open device. " << std::endl;
	// 配置并启动设备
	k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
	config.camera_fps = K4A_FRAMES_PER_SECOND_30;
	config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
	config.color_resolution = K4A_COLOR_RESOLUTION_720P;
	config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
	config.synchronized_images_only = true;
	device.start_cameras(&config);
	std::cout << "Done: start camera." << std::endl;
	// 稳定化
	k4a::capture capture;
	int iAuto = 0;//用来稳定，类似自动曝光
	int iAutoError = 0;// 统计自动曝光的失败次数
	while (true) {
		if (device.get_capture(&capture)) {
			std::cout << iAuto << "Capture several frames to give auto-exposure" <<
				std::endl;
			// 跳过前 n 个（成功的数据采集）循环，用来稳定
			if (iAuto != 30) {
				iAuto++;
				continue;
			}
			else {
				std::cout << "Done: auto-exposure" << std::endl;
				break;// 跳出该循环，完成相机的稳定过程
			}
		}
		else {
			std::cout << iAutoError << ". K4A_WAIT_RESULT_TIMEOUT." <<
				std::endl;
			if (iAutoError != 30) {
				iAutoError++;
				continue;
			}
			else {
				std::cout << "Error: failed to give auto-exposure. " << std::endl;
				return -1;
			}
		}
	}
	std::cout << "--------------------------------------" << std::endl;
	std::cout << "---- Have Started Azure Kinect DK ----" << std::endl;
	std::cout << "--------------------------------------" << std::endl;
	// 从设备获取捕获
	k4a::image rgbImage;
	k4a::image depthImage;
	k4a::image transformed_depthImage;
	cv::Mat cv_rgbImage_with_alpha;
	cv::Mat cv_rgbImage_no_alpha;
	cv::Mat cv_depth;
	cv::Mat cv_depth_8U;
	int index = 0;
	while (index < 1)
	{
		if (device.get_capture(&capture)) {
			// rgb
			rgbImage = capture.get_color_image();
			// depth
			depthImage = capture.get_depth_image();
			//获取彩色点云
			k4a::calibration k4aCalibration =
				device.get_calibration(config.depth_mode, config.color_resolution);
			k4a::transformation k4aTransformation =
				k4a::transformation(k4aCalibration);
			int color_image_width_pixels = rgbImage.get_width_pixels();
			int color_image_height_pixels = rgbImage.get_height_pixels();
			transformed_depthImage = NULL;
			transformed_depthImage =
				k4a::image::create(K4A_IMAGE_FORMAT_DEPTH16, color_image_width_pixels, color_image_height_pixels, color_image_width_pixels * (int)sizeof(uint16_t));
			k4a::image point_cloud_image = NULL;
			point_cloud_image =
				k4a::image::create(K4A_IMAGE_FORMAT_CUSTOM, color_image_width_pixels, color_image_height_pixels, color_image_width_pixels * 3 * (int)sizeof(int16_t));
			if (depthImage.get_width_pixels() == rgbImage.get_width_pixels() &&
				depthImage.get_height_pixels() == rgbImage.get_height_pixels()) {
				std::copy(depthImage.get_buffer(), depthImage.get_buffer() + depthImage.get_height_pixels() *
					depthImage.get_width_pixels() * (int)sizeof(uint16_t), transformed_depthImage.get_buffer());
			}
			else {
				k4aTransformation.depth_image_to_color_camera(depthImage, &transformed_depthImage);
			}
			k4aTransformation.depth_image_to_point_cloud(transformed_depthImage, K4A_CALIBRATION_TYPE_COLOR, &point_cloud_image);
			pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new
				pcl::PointCloud<pcl::PointXYZRGB>);
			cloud->width = color_image_width_pixels;
			cloud->height = color_image_height_pixels;
			cloud->is_dense = false;
			cloud->resize(static_cast<size_t>(color_image_width_pixels) *
				color_image_height_pixels);
			const int16_t* point_cloud_image_data = reinterpret_cast<const
				int16_t*>(point_cloud_image.get_buffer());
			const uint8_t* color_image_data = rgbImage.get_buffer();
			for (int i = 0; i < color_image_width_pixels * color_image_height_pixels;
				i++) {
				PointT point;
				point.x = point_cloud_image_data[3 * i + 0] / 1000.0f;
				point.y = point_cloud_image_data[3 * i + 1] / 1000.0f;
				point.z = point_cloud_image_data[3 * i + 2] / 1000.0f;
				point.b = color_image_data[4 * i + 0];
				point.g = color_image_data[4 * i + 1];
				point.r = color_image_data[4 * i + 2];
				uint8_t alpha = color_image_data[4 * i + 3];
				if (point.x == 0 && point.y == 0 && point.z == 0 && alpha == 0)
					continue;
				cloud->points[i] = point;
			}
			std::cout << "保存点云数据。" << endl;
			pcl::io::savePLYFile("./fusedCloud.ply", *cloud); //将点云数据保存为 ply 文件
			pcl::io::savePCDFile("./fusedCloud.pcd", *cloud); //将点云数据保存为 ply 文件
		}
		else {
			std::cout << "false: K4A_WAIT_RESULT_TIMEOUT." << std::endl;
		}
		index++;
	}
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new
		pcl::PointCloud<pcl::PointXYZ>);
	char strfilepath[256] = "./fusedCloud.pcd";
	//打开点云文件
	if (-1 == pcl::io::loadPCDFile(strfilepath, *cloud))
	{
		std::cout << "error input!" << std::endl;
		return -1;
	}
	std::cout << cloud->points.size() << std::endl;
	pcl::visualization::CloudViewer viewer("Cloud Viewer");
	viewer.showCloud(cloud);
	system("pause");
	cv::destroyAllWindows();
	// 释放，关闭设备
	rgbImage.reset();
	depthImage.reset();
	capture.reset();
	device.close();
	return 0;
}