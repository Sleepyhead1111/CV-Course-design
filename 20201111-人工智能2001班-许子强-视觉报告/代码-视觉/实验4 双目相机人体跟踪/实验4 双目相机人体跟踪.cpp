#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <k4a/k4a.h>
#include <k4abt.h>
#include <math.h>
// OpenCV
#include <opencv2/opencv.hpp>
// Azure Kinect DK
#include <k4a/k4a.hpp>
#define VERIFY(result, error) if(result != K4A_RESULT_SUCCEEDED) { printf("%s \n - (File: %s, Function: %s, Line: %d)\n", error, __FILE__, __FUNCTION__, __LINE__); exit(1);}
int main()
{
//定义身高
float UPPER_BODY; //上身长度
float body_angel; //上身倾角
k4a_device_t device = NULL;
VERIFY(k4a_device_open(0, &device), "Open K4A Device failed!");
const uint32_t device_count = k4a_device_get_installed_count();
if (1 == device_count)
{
	std::cout << "Found " << device_count << " connected devices. " <<
		std::endl;
}
else
{
	std::cout << "Error: more than one K4A devices found. " << std::endl;
}
//打开设备
k4a_device_open(0, &device);
std::cout << "Done: open device. " << std::endl;
// 配置相机，可以根据需要自行修改
k4a_device_configuration_t deviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
deviceConfig.depth_mode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
deviceConfig.color_resolution = K4A_COLOR_RESOLUTION_720P;
deviceConfig.camera_fps = K4A_FRAMES_PER_SECOND_30;
deviceConfig.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
deviceConfig.synchronized_images_only = true;
//开始相机
VERIFY(k4a_device_start_cameras(device, &deviceConfig), "Start K4Acameras failed!");
std::cout << "Done: start camera." << std::endl;
//查询传感器校准
k4a_calibration_t sensor_calibration;
k4a_device_get_calibration(device, deviceConfig.depth_mode, deviceConfig.color_resolution, &sensor_calibration);
VERIFY(k4a_device_get_calibration(device, deviceConfig.depth_mode, deviceConfig.color_resolution, &sensor_calibration), "Get depth camera calibration failed!");
//创建人体跟踪器
k4abt_tracker_t tracker = NULL;
k4abt_tracker_configuration_t tracker_config = K4ABT_TRACKER_CONFIG_DEFAULT;
k4abt_tracker_create(&sensor_calibration, tracker_config, &tracker);
VERIFY(k4abt_tracker_create(&sensor_calibration, tracker_config, &tracker), "Body tracker initialization failed!");
cv::Mat cv_rgbImage_with_alpha;
cv::Mat cv_rgbImage_no_alpha;
cv::Mat cv_depth;
cv::Mat cv_depth_8U;
int frame_count = 0;
while (true)
{
	k4a_capture_t sensor_capture;
	k4a_wait_result_t get_capture_result = k4a_device_get_capture(device, &sensor_capture, K4A_WAIT_INFINITE);
	//获取 RGB 和 depth 图像
	k4a_image_t rgbImage = k4a_capture_get_color_image(sensor_capture);
	k4a_image_t depthImage = k4a_capture_get_depth_image(sensor_capture);
	//RGB
	cv_rgbImage_with_alpha =
	cv::Mat(k4a_image_get_height_pixels(rgbImage), k4a_image_get_width_pixels(rgbImage), CV_8UC4, k4a_image_get_buffer(rgbImage));
	cvtColor(cv_rgbImage_with_alpha, cv_rgbImage_no_alpha, cv::COLOR_BGRA2BGR);
	//depth
	cv_depth = cv::Mat(k4a_image_get_height_pixels(depthImage), k4a_image_get_width_pixels(depthImage), CV_16U, k4a_image_get_buffer(depthImage), k4a_image_get_stride_bytes(depthImage));
	cv_depth.convertTo(cv_depth_8U, CV_8U, 1);
	//计算姿态
	if (get_capture_result == K4A_WAIT_RESULT_SUCCEEDED)
	{
		frame_count++;
		k4a_wait_result_t queue_capture_result = k4abt_tracker_enqueue_capture(tracker, sensor_capture, K4A_WAIT_INFINITE);
		k4a_capture_release(sensor_capture); // 请记住，一旦使用完毕，请释放传感器捕获
			if (queue_capture_result == K4A_WAIT_RESULT_TIMEOUT)
			{
				printf("Error! Add capture to tracker process queue timeout!\n");
				break;
			}
			else if (queue_capture_result == K4A_WAIT_RESULT_FAILED)
			{
				printf("Error! Add capture to tracker process queue failed!\n");
				break;
			}
		k4abt_frame_t body_frame = NULL;
		k4a_wait_result_t pop_frame_result = k4abt_tracker_pop_result(tracker, &body_frame, K4A_WAIT_INFINITE);
		if (pop_frame_result == K4A_WAIT_RESULT_SUCCEEDED)
		{
			// 成功身体跟踪结果，开始处理
			size_t num_bodies = k4abt_frame_get_num_bodies(body_frame);
			printf("%zu bodies are detected!\n", num_bodies);
			for (size_t i = 0; i < num_bodies; i++)
			{
				k4abt_skeleton_t skeleton;
				k4abt_frame_get_body_skeleton(body_frame, i, &skeleton);
				std::cout << typeid(skeleton.joints->position.v).name();
				k4a_float2_t P_HEAD_2D;
				k4a_float2_t P_NECK_2D;
				k4a_float2_t P_CHEST_2D;
				k4a_float2_t P_HIP_2D;
				k4a_float2_t P_CLAVICLE_RIGHT_2D;
				k4a_float2_t P_CLAVICLE_LEFT_2D;
				k4a_float2_t P_HIP_RIGHT_2D;
				k4a_float2_t P_HIP_LEFT_2D;
				k4a_float2_t P_KNEE_LEFT_2D;
				k4a_float2_t P_KNEE_RIGHT_2D;
				int result;
				//头部
				k4abt_joint_t P_HEAD = skeleton.joints[K4ABT_JOINT_NOSE];
				//3D 转 2D，并在 color 中画出
				k4a_calibration_3d_to_2d(&sensor_calibration, &P_HEAD.position, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_COLOR, &P_HEAD_2D, &result);
				cv::circle(cv_rgbImage_no_alpha, cv::Point(P_HEAD_2D.xy.x, P_HEAD_2D.xy.y), 3, cv::Scalar(0, 255, 255));



				//颈部
				/*自己完成代码*/
				k4abt_joint_t P_NECK = skeleton.joints[K4ABT_JOINT_NECK];
				//3D 转 2D，并在 color 中画出
				/*自己完成代码*/
				k4a_calibration_3d_to_2d(&sensor_calibration, &P_NECK.position, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_COLOR, &P_NECK_2D, &result);
				cv::circle(cv_rgbImage_no_alpha, cv::Point(P_NECK_2D.xy.x, P_NECK_2D.xy.y), 3, cv::Scalar(0, 255, 255));
				//胸部
				/*自己完成代码*/
				k4abt_joint_t P_CHEST = skeleton.joints[K4ABT_JOINT_SPINE_CHEST];
				//3D 转 2D，并在 color 中画出
				/*自己完成代码*/
				k4a_calibration_3d_to_2d(&sensor_calibration, &P_CHEST.position, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_COLOR, &P_CHEST_2D, &result);
				cv::circle(cv_rgbImage_no_alpha, cv::Point(P_CHEST_2D.xy.x, P_CHEST_2D.xy.y), 3, cv::Scalar(0, 255, 255));
				//髋部
				/*自己完成代码*/
				k4abt_joint_t P_HIP = skeleton.joints[K4ABT_JOINT_PELVIS];
				//3D 转 2D，并在 color 中画出
				/*自己完成代码*/
				k4a_calibration_3d_to_2d(&sensor_calibration, &P_HIP.position, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_COLOR, &P_HIP_2D, &result);
				cv::circle(cv_rgbImage_no_alpha, cv::Point(P_HIP_2D.xy.x, P_HIP_2D.xy.y), 3, cv::Scalar(0, 255, 255));
				//右肩（计算上身倾角需要）
				/*自己完成代码*/
				k4abt_joint_t P_CLAVICLE_RIGHT = skeleton.joints[K4ABT_JOINT_CLAVICLE_RIGHT];
				//3D 转 2D，并在 color 中画出
				/*自己完成代码*/
				k4a_calibration_3d_to_2d(&sensor_calibration, &P_CLAVICLE_RIGHT.position, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_COLOR, &P_CLAVICLE_RIGHT_2D, &result);
				cv::circle(cv_rgbImage_no_alpha, cv::Point(P_CLAVICLE_RIGHT_2D.xy.x, P_CLAVICLE_RIGHT_2D.xy.y), 3, cv::Scalar(0, 255, 255));


				
				//右髋（计算上身倾角需要）
				k4abt_joint_t P_HIP_RIGHT = skeleton.joints[K4ABT_JOINT_HIP_RIGHT];
				//3D 转 2D，并在 color 中画出,并画出右肩到右髋的连线
				k4a_calibration_3d_to_2d(&sensor_calibration, &P_HIP_RIGHT.position, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_COLOR, &P_HIP_RIGHT_2D, &result);
				cv::circle(cv_rgbImage_no_alpha, cv::Point(P_HIP_RIGHT_2D.xy.x, P_HIP_RIGHT_2D.xy.y), 3, cv::Scalar(0, 255, 255));
				cv::line(cv_rgbImage_no_alpha, cv::Point(P_CLAVICLE_RIGHT_2D.xy.x, P_CLAVICLE_RIGHT_2D.xy.y), cv::Point(P_HIP_RIGHT_2D.xy.x, P_HIP_RIGHT_2D.xy.y), cv::Scalar(0, 0, 255), 2);
				

				
				//右膝（计算上身倾角需要）
				/*自己完成代码*/
				k4abt_joint_t P_KNEE_RIGHT = skeleton.joints[K4ABT_JOINT_KNEE_RIGHT];
				//3D 转 2D，并在 color 中画出,并画出右肩到右膝、右髋到右膝的连线
				/*自己完成代码*/
				k4a_calibration_3d_to_2d(&sensor_calibration, &P_KNEE_RIGHT.position, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_COLOR, &P_KNEE_RIGHT_2D, &result);
				cv::circle(cv_rgbImage_no_alpha, cv::Point(P_KNEE_RIGHT_2D.xy.x, P_KNEE_RIGHT_2D.xy.y), 3, cv::Scalar(0, 255, 255));
				cv::line(cv_rgbImage_no_alpha, cv::Point(P_CLAVICLE_RIGHT_2D.xy.x, P_CLAVICLE_RIGHT_2D.xy.y), cv::Point(P_KNEE_RIGHT_2D.xy.x, P_KNEE_RIGHT_2D.xy.y), cv::Scalar(0, 0, 255), 2);
				cv::line(cv_rgbImage_no_alpha, cv::Point(P_HIP_RIGHT_2D.xy.x, P_HIP_RIGHT_2D.xy.y), cv::Point(P_KNEE_RIGHT_2D.xy.x, P_KNEE_RIGHT_2D.xy.y), cv::Scalar(0, 0, 255), 2);

				//左肩（计算上身倾角需要）
				/*自己完成代码*/
				k4abt_joint_t P_CLAVICLE_LEFT = skeleton.joints[K4ABT_JOINT_CLAVICLE_LEFT];
				//3D 转 2D，并在 color 中画出
				/*自己完成代码*/
				k4a_calibration_3d_to_2d(&sensor_calibration, &P_CLAVICLE_LEFT.position, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_COLOR, &P_CLAVICLE_LEFT_2D, &result);
				cv::circle(cv_rgbImage_no_alpha, cv::Point(P_CLAVICLE_LEFT_2D.xy.x, P_CLAVICLE_LEFT_2D.xy.y), 3, cv::Scalar(0, 255, 255));

				//左髋（计算上身倾角需要）
				/*自己完成代码*/
				k4abt_joint_t P_HIP_LEFT = skeleton.joints[K4ABT_JOINT_HIP_LEFT];
				//3D 转 2D，并在 color 中画出,并画出左肩到左髋的连线
				/*自己完成代码*/
				k4a_calibration_3d_to_2d(&sensor_calibration, &P_HIP_LEFT.position, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_COLOR, &P_HIP_LEFT_2D, &result);
				cv::circle(cv_rgbImage_no_alpha, cv::Point(P_HIP_LEFT_2D.xy.x, P_HIP_LEFT_2D.xy.y), 3, cv::Scalar(0, 255, 255));
				cv::line(cv_rgbImage_no_alpha, cv::Point(P_CLAVICLE_LEFT_2D.xy.x, P_CLAVICLE_LEFT_2D.xy.y), cv::Point(P_HIP_LEFT_2D.xy.x, P_HIP_LEFT_2D.xy.y), cv::Scalar(0, 0, 255), 2);

				//左膝（计算上身倾角需要）
				/*自己完成代码*/
				k4abt_joint_t P_KNEE_LEFT = skeleton.joints[K4ABT_JOINT_KNEE_LEFT];
				//3D 转 2D，并在 color 中画出,并画出左肩到左膝、左髋到左膝的连线
				/*自己完成代码*/
				k4a_calibration_3d_to_2d(&sensor_calibration, &P_KNEE_LEFT.position, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_COLOR, &P_KNEE_LEFT_2D, &result);
				cv::circle(cv_rgbImage_no_alpha, cv::Point(P_KNEE_LEFT_2D.xy.x, P_KNEE_LEFT_2D.xy.y), 3, cv::Scalar(0, 255, 255));
				cv::line(cv_rgbImage_no_alpha, cv::Point(P_CLAVICLE_LEFT_2D.xy.x, P_CLAVICLE_LEFT_2D.xy.y), cv::Point(P_KNEE_LEFT_2D.xy.x, P_KNEE_LEFT_2D.xy.y), cv::Scalar(0, 0, 255), 2);
				cv::line(cv_rgbImage_no_alpha, cv::Point(P_HIP_LEFT_2D.xy.x, P_HIP_LEFT_2D.xy.y), cv::Point(P_KNEE_LEFT_2D.xy.x, P_KNEE_LEFT_2D.xy.y), cv::Scalar(0, 0, 255), 2);



				//输出头部关键点坐标（skeleton.joints_head->position.v为头部坐标点，数据结构 float[3]）
				std::cout << "头部坐标：";
				for (size_t i = 0; i < 3; i++)
				{
					std::cout << P_HEAD.position.v[i] << " ";
				}
				printf("\n");



				//输出颈部关键点坐标
				/*自己完成代码*/
				std::cout << "颈部坐标：";
				for (size_t i = 0; i < 3; i++)
				{
					std::cout << P_NECK.position.v[i] << " ";
				}
				printf("\n");

				//输出胸部关键点坐标
				/*自己完成代码*/
				std::cout << "胸部坐标：";
				for (size_t i = 0; i < 3; i++)
				{
					std::cout << P_CHEST.position.v[i] << " ";
				}
				printf("\n");

				//输出髋部关键点坐标
				/*自己完成代码*/
				std::cout << "髋部坐标：";
				for (size_t i = 0; i < 3; i++)
				{
					std::cout << P_HIP.position.v[i] << " ";
				}
				printf("\n");



				//估算人体身高
				UPPER_BODY = sqrt(pow((P_NECK.position.xyz.x - P_HIP.position.xyz.x), 2)
					+ pow((P_NECK.position.xyz.y - P_HIP.position.xyz.y), 2)
					+ pow((P_NECK.position.xyz.z - P_HIP.position.xyz.z), 2));
				float HEIGHT = UPPER_BODY * 1770 / 518;
				std::cout << "估算身高高度是;" << HEIGHT << std::endl;
				//估算上身倾角
				float ang_clavicetohip_right =
					sqrt(pow((P_CLAVICLE_RIGHT.position.xyz.x - P_HIP_RIGHT.position.xyz.x), 2)
						+ pow((P_CLAVICLE_RIGHT.position.xyz.y - P_HIP_RIGHT.position.xyz.y), 2) +
						pow((P_CLAVICLE_RIGHT.position.xyz.z - P_HIP_RIGHT.position.xyz.z), 2));
				float ang_clavicetohip_left =
					sqrt(pow((P_CLAVICLE_LEFT.position.xyz.x - P_HIP_LEFT.position.xyz.x), 2) +
						pow((P_CLAVICLE_LEFT.position.xyz.y - P_HIP_LEFT.position.xyz.y), 2) +
						pow((P_CLAVICLE_LEFT.position.xyz.z - P_HIP_LEFT.position.xyz.z), 2));
				float ang_hiptoknee_right =
					sqrt(pow((P_HIP_RIGHT.position.xyz.x - P_KNEE_RIGHT.position.xyz.x), 2) +
						pow((P_HIP_RIGHT.position.xyz.y - P_KNEE_RIGHT.position.xyz.y), 2) +
						pow((P_HIP_RIGHT.position.xyz.z - P_KNEE_RIGHT.position.xyz.z), 2));
				float ang_hiptoknee_left =
					sqrt(pow((P_HIP_LEFT.position.xyz.x - P_KNEE_LEFT.position.xyz.x), 2) +
						pow((P_HIP_LEFT.position.xyz.y - P_KNEE_LEFT.position.xyz.y), 2) +
						pow((P_HIP_LEFT.position.xyz.z - P_KNEE_LEFT.position.xyz.z), 2));
				float ang_clavicetoknee_right =
					sqrt(pow((P_CLAVICLE_RIGHT.position.xyz.x - P_KNEE_RIGHT.position.xyz.x), 2) + pow((P_CLAVICLE_RIGHT.position.xyz.y - P_KNEE_RIGHT.position.xyz.y), 2) + pow((P_CLAVICLE_RIGHT.position.xyz.z - P_KNEE_RIGHT.position.xyz.z), 2));
				float ang_clavicetoknee_left =
					sqrt(pow((P_CLAVICLE_LEFT.position.xyz.x - P_KNEE_LEFT.position.xyz.x), 2) +
						pow((P_CLAVICLE_LEFT.position.xyz.y - P_KNEE_LEFT.position.xyz.y), 2) +
						pow((P_CLAVICLE_LEFT.position.xyz.z - P_KNEE_LEFT.position.xyz.z), 2));
				float body_angel_right = acos((ang_clavicetohip_right *
					ang_clavicetohip_right + ang_hiptoknee_right * ang_hiptoknee_right - ang_clavicetoknee_right * ang_clavicetoknee_right) / (2 * ang_clavicetohip_right *
						ang_hiptoknee_right)) * 180.0 / 3.1415926;
				float body_angel_left = acos((ang_clavicetohip_left *
					ang_clavicetohip_left + ang_hiptoknee_left * ang_hiptoknee_left - ang_clavicetoknee_left * ang_clavicetoknee_left) / (2 * ang_clavicetohip_left *
						ang_hiptoknee_left)) * 180.0 / 3.1415926;
				body_angel = (body_angel_right + body_angel_left) / 2;
				std::cout << " 估 算 上 身 倾 角 是 ;" << body_angel << std::endl;
				uint32_t id = k4abt_frame_get_body_id(body_frame, i);

				//加油识别
				k4abt_joint_t P_SHOULDER_RIGHT = skeleton.joints[K4ABT_JOINT_SHOULDER_RIGHT];
				k4abt_joint_t P_ELBOW_RIGHT = skeleton.joints[K4ABT_JOINT_ELBOW_RIGHT];
				k4abt_joint_t P_WRIST_RIGHT = skeleton.joints[K4ABT_JOINT_WRIST_RIGHT];

				float P_SHOULDER_RIGHT_X = P_SHOULDER_RIGHT.position.xyz.x;
				float P_SHOULDER_RIGHT_Y = P_SHOULDER_RIGHT.position.xyz.y;
				float P_SHOULDER_RIGHT_Z = P_SHOULDER_RIGHT.position.xyz.z;

				float P_ELBOW_RIGHT_X = P_ELBOW_RIGHT.position.xyz.x;
				float P_ELBOW_RIGHT_Y = P_ELBOW_RIGHT.position.xyz.y;
				float P_ELBOW_RIGHT_Z = P_ELBOW_RIGHT.position.xyz.z;

				float P_WRIST_RIGHT_X = P_WRIST_RIGHT.position.xyz.x;
				float P_WRIST_RIGHT_Y = P_WRIST_RIGHT.position.xyz.y;
				float P_WRIST_RIGHT_Z = P_WRIST_RIGHT.position.xyz.z;

				float P_CLAVICLE_RIGHT_X = P_CLAVICLE_RIGHT.position.xyz.x;
				float P_CLAVICLE_RIGHT_Y = P_CLAVICLE_RIGHT.position.xyz.y;
				float P_CLAVICLE_RIGHT_Z = P_CLAVICLE_RIGHT.position.xyz.z;

				double P_SHOULDER_RIGHT_P_ELBOW_RIGHT = sqrt((double)(
					(P_SHOULDER_RIGHT_X - P_ELBOW_RIGHT_X) * (P_SHOULDER_RIGHT_X - P_ELBOW_RIGHT_X)
					+ (P_SHOULDER_RIGHT_Y - P_ELBOW_RIGHT_Y) * (P_SHOULDER_RIGHT_Y - P_ELBOW_RIGHT_Y)
					+ (P_SHOULDER_RIGHT_Z - P_ELBOW_RIGHT_Z) * (P_SHOULDER_RIGHT_Z - P_ELBOW_RIGHT_Z))); //两点之间连线

				double P_SHOULDER_RIGHT_P_WRIST_RIGHT = sqrt((double)(
					(P_SHOULDER_RIGHT_X - P_WRIST_RIGHT_X) * (P_SHOULDER_RIGHT_X - P_WRIST_RIGHT_X)
					+ (P_SHOULDER_RIGHT_Y - P_WRIST_RIGHT_Y) * (P_SHOULDER_RIGHT_Y - P_WRIST_RIGHT_Y)
					+ (P_SHOULDER_RIGHT_Z - P_WRIST_RIGHT_Z) * (P_SHOULDER_RIGHT_Z - P_WRIST_RIGHT_Z))); //两点之间连线

				double P_ELBOW_RIGHT_P_WRIST_RIGHT = sqrt((double)(
					(P_ELBOW_RIGHT_X - P_WRIST_RIGHT_X) * (P_ELBOW_RIGHT_X - P_WRIST_RIGHT_X)
					+ (P_ELBOW_RIGHT_Y - P_WRIST_RIGHT_Y) * (P_ELBOW_RIGHT_Y - P_WRIST_RIGHT_Y)
					+ (P_ELBOW_RIGHT_Z - P_WRIST_RIGHT_Z) * (P_ELBOW_RIGHT_Z - P_WRIST_RIGHT_Z))); //两点之间连线

				double angle = acos(
					(P_SHOULDER_RIGHT_P_ELBOW_RIGHT * P_SHOULDER_RIGHT_P_ELBOW_RIGHT
						+ P_ELBOW_RIGHT_P_WRIST_RIGHT * P_ELBOW_RIGHT_P_WRIST_RIGHT
						- P_SHOULDER_RIGHT_P_WRIST_RIGHT * P_SHOULDER_RIGHT_P_WRIST_RIGHT)
					/ (2 * P_SHOULDER_RIGHT_P_ELBOW_RIGHT * P_ELBOW_RIGHT_P_WRIST_RIGHT)) * 180.0 / 3.1415926;
				printf("夹角为：%f\n", angle);
				if (angle < 100)
				{
					std::cout << "加油！" << std::endl;
				}

			}
			printf("%zu bodies are detected!\n", num_bodies);
			k4abt_frame_release(body_frame); // 使用完毕后，请记住释放身体框架
		}
		else if (pop_frame_result == K4A_WAIT_RESULT_TIMEOUT)
		{
			printf("Error! Pop body frame result timeout!\n");
			break;
		}
		else
		{
			printf("Pop body frame result failed!\n");
			break;
		}
	}
	else if (get_capture_result == K4A_WAIT_RESULT_TIMEOUT)
	{
		printf("Error! Get depth frame time out!\n");
		break;
	}
	else
	{
		printf("Get depth capture returned error: %d\n", get_capture_result);
		break;
	}
	imshow("color", cv_rgbImage_no_alpha);
	imshow("depth", cv_depth_8U);
	k4a_image_release(rgbImage);
	k4a_image_release(depthImage);
	if (cv::waitKey(1) == 27)
		break;
}
printf("Finished body tracking processing!\n");
k4abt_tracker_shutdown(tracker);
k4abt_tracker_destroy(tracker);
k4a_device_stop_cameras(device);
k4a_device_close(device);
return 0;
}