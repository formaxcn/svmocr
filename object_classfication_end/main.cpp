#include<iostream>
#include<map>
#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<string>
#include<opencv2/ml/ml.hpp>
#include<opencv2/features2d/features2d.hpp>
#include<opencv2/nonfree/features2d.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<fstream>
//boost 库
#include<boost/filesystem.hpp>

#include"Config.h"

using namespace cv;
using namespace std;
//定义一个boost库的命名空间
namespace fs = boost::filesystem;
using namespace fs;


class categorizer
{
private:
	//从类目名称到数据的map映射
	map<string, Mat> result_objects;
	//存放所有训练图片的BOW
	map<string, Mat> allsamples_bow;
	//从类目名称到训练图集的映射，关键字可以重复出现
	multimap<string, Mat> train_set;
	// 训练得到的SVM
	CvSVM *stor_svms;
	//类目名称，也就是TRAIN_FOLDER设置的目录名
	vector<string> category_name;
	//类目数目
	int categories_size;
	//用SURF特征构造视觉词库的聚类数目
	int clusters;
	//存放训练图片词典
	Mat vocab;

	//特征检测器detectors与描述子提取器extractors   泛型句柄类Ptr
	Ptr<FeatureDetector> featureDecter;
	Ptr<DescriptorExtractor> descriptorExtractor;

	Ptr<BOWKMeansTrainer> bowtrainer;
	Ptr<BOWImgDescriptorExtractor> bowDescriptorExtractor;
	Ptr<FlannBasedMatcher> descriptorMacher;

	//构造训练集合
	void make_train_set();
	// 移除扩展名，用来讲模板组织成类目
	string remove_extention(string);

public:
	//构造函数
	categorizer(int);
	// 聚类得出词典
	void bulid_vacab();
	//构造BOW
	void compute_bow_image();
	//训练分类器
	void trainSvm();
	//将测试图片分类
	void category_By_svm();
};

// 移除扩展名，用来讲模板组织成类目
string categorizer::remove_extention(string full_name)
{
	//find_last_of找出字符最后一次出现的地方
	int last_index = full_name.find_last_of(".");
	string name = full_name.substr(0, last_index);
	return name;
}

// 构造函数
categorizer::categorizer(int _clusters)
{
	cout << "开始初始化..." << endl;
	clusters = _clusters;
	//初始化指针
	featureDecter = new SurfFeatureDetector();
	descriptorExtractor = new SurfDescriptorExtractor();

	bowtrainer = new BOWKMeansTrainer(clusters);
	descriptorMacher = new FlannBasedMatcher();
	bowDescriptorExtractor = new BOWImgDescriptorExtractor(descriptorExtractor, descriptorMacher);

	//boost库文件 遍历数据文件夹  directory_iterator(p)就是迭代器的起点，无参数的directory_iterator()就是迭代器的终点。
	directory_iterator begin_iter(TEMPLATE_FOLDER);
	directory_iterator end_iter;
	//获取该目录下的所有文件名
	for (; begin_iter != end_iter; ++begin_iter)
	{
		string filename = string(TEMPLATE_FOLDER) + begin_iter->path().filename().string();
		string sub_category = remove_extention(begin_iter->path().filename().string());
		//读入模板图片
		Mat image = imread(filename);
		Mat templ_image;

		//存储原图模板
		result_objects[sub_category] = image;
	}

	cout << "初始化完毕..." << endl;
	//读取训练集
	make_train_set();
}


//构造训练集合
void categorizer::make_train_set()
{
	cout << "读取训练集..." << endl;
	string categor;
	//递归迭代rescursive 直接定义两个迭代器：i为迭代起点（有参数），end_iter迭代终点
	for (recursive_directory_iterator i(TRAIN_FOLDER), end_iter; i != end_iter; i++)
	{
		// level == 0即为目录，因为TRAIN__FOLDER中设置如此
		if (i.level() == 0)
		{
			// 将类目名称设置为目录的名称
			categor = (i->path()).filename().string();
			category_name.push_back(categor);

		}
		else
		{
			// 读取文件夹下的文件。level 1表示这是一副训练图，通过multimap容器来建立由类目名称到训练图的一对多的映射
			string filename = string(TRAIN_FOLDER) + categor + string("/") + (i->path()).filename().string();
			Mat temp = imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
			pair<string, Mat> p(categor, temp);

			//得到训练集
			train_set.insert(p);
		}
	}

	categories_size = category_name.size();
	cout << "发现 " << categories_size << "种类别物体..." << endl;
}


// 训练图片feature聚类，得出词典
void categorizer::bulid_vacab()
{
	FileStorage vacab_fs(DATA_FOLDER "vocab.xml", FileStorage::READ);

	//如果之前已经生成好，就不需要重新聚类生成词典
	if (vacab_fs.isOpened())
	{
		cout << "图片已经聚类，词典已经存在.." << endl;
		vacab_fs.release();
	}
	else
	{
		Mat vocab_descriptors;

		// 对于每一幅模板，提取SURF算子，存入到vocab_descriptors中
		multimap<string, Mat> ::iterator i = train_set.begin();
		for (; i != train_set.end(); i++)
		{
			vector<KeyPoint>kp;
			Mat templ = (*i).second;
			Mat descrip;
			featureDecter->detect(templ, kp);

			descriptorExtractor->compute(templ, kp, descrip);
			//push_back(Mat);在原来的Mat的最后一行后再加几行,元素为Mat时， 其类型和列的数目 必须和矩阵容器是相同的
			vocab_descriptors.push_back(descrip);
		}
		cout << "训练图片开始聚类..." << endl;
		//将每一副图的Surf特征利用add函数加入到bowTraining中去,就可以进行聚类训练了
		bowtrainer->add(vocab_descriptors);
		// 对SURF描述子进行聚类
		vocab = bowtrainer->cluster();
		cout << "聚类完毕，得出词典..." << endl;

		//以文件格式保存词典
		FileStorage file_stor(DATA_FOLDER "vocab.xml", FileStorage::WRITE);
		file_stor << "vocabulary" << vocab;
		file_stor.release();

	}
}


//构造bag of words
void categorizer::compute_bow_image()
{
	cout << "构造bag of words..." << endl;
	FileStorage va_fs(DATA_FOLDER "vocab.xml", FileStorage::READ);

	//如果词典存在则直接读取
	if (va_fs.isOpened())
	{
		Mat temp_vacab;
		va_fs["vocabulary"] >> temp_vacab;
		bowDescriptorExtractor->setVocabulary(temp_vacab);
		va_fs.release();

	}
	else
	{
		//对每张图片的特征点，统计这张图片各个类别出现的频率，作为这张图片的bag of words
		bowDescriptorExtractor->setVocabulary(vocab);
	}

	//如果bow.txt已经存在说明之前已经训练过了，下面就不用重新构造BOW
	string bow_path = string(DATA_FOLDER) + string("bow.txt");
	fs::ifstream read_file(bow_path);

	//如BOW已经存在，则不需要构造
	if (read_file)
	{
		cout << "BOW 已经准备好..." << endl;
	}
	else{

		// 对于每一幅模板，提取SURF算子，存入到vocab_descriptors中
		multimap<string, Mat> ::iterator i = train_set.begin();

		for (; i != train_set.end(); i++)
		{
			vector<KeyPoint>kp;
			string cate_nam = (*i).first;
			Mat tem_image = (*i).second;
			Mat imageDescriptor;
			featureDecter->detect(tem_image, kp);

			bowDescriptorExtractor->compute(tem_image, kp, imageDescriptor);
			//push_back(Mat);在原来的Mat的最后一行后再加几行,元素为Mat时， 其类型和列的数目 必须和矩阵容器是相同的
			allsamples_bow[cate_nam].push_back(imageDescriptor);
		}
		//简单输出一个文本，为后面判断做准备
		fs::ofstream ous(bow_path);
		ous << "flag";
		cout << "bag of words构造完毕..." << endl;

	}
}

//训练分类器

void categorizer::trainSvm()
{
	int flag = 0;
	for (int k = 0; k < categories_size; k++)
	{
		string svm_file_path = string(DATA_FOLDER) + category_name[k] + string("SVM.xml");
		FileStorage svm_fil(svm_file_path, FileStorage::READ);
		//判断训练结果是否存在
		if (svm_fil.isOpened())
		{
			svm_fil.release();
			continue;
		}
		else
		{
			flag = -1;
			break;
		}

	}

	//如果训练结果已经存在则不需要重新训练
	if (flag != -1)
	{
		cout << "分类器已经训练完毕..." << endl;
	}
	else

	{
		stor_svms = new CvSVM[categories_size];
		//设置训练参数
		SVMParams svmParams;
		svmParams.svm_type = CvSVM::C_SVC;
		svmParams.kernel_type = CvSVM::LINEAR;
		svmParams.term_crit = cvTermCriteria(CV_TERMCRIT_ITER, 100, 1e-6);

		cout << "训练分类器..." << endl;
		for (int i = 0; i < categories_size; i++)
		{
			Mat tem_Samples(0, allsamples_bow.at(category_name[i]).cols, allsamples_bow.at(category_name[i]).type());
			Mat responses(0, 1, CV_32SC1);
			tem_Samples.push_back(allsamples_bow.at(category_name[i]));
			Mat posResponses(allsamples_bow.at(category_name[i]).rows, 1, CV_32SC1, Scalar::all(1));
			responses.push_back(posResponses);

			for (auto itr = allsamples_bow.begin(); itr != allsamples_bow.end(); ++itr)
			{
				if (itr->first == category_name[i]) {
					continue;
				}
				tem_Samples.push_back(itr->second);
				Mat response(itr->second.rows, 1, CV_32SC1, Scalar::all(-1));
				responses.push_back(response);

			}

			stor_svms[i].train(tem_Samples, responses, Mat(), Mat(), svmParams);
			//存储svm
			string svm_filename = string(DATA_FOLDER) + category_name[i] + string("SVM.xml");
			stor_svms[i].save(svm_filename.c_str());
		}

		cout << "分类器训练完毕..." << endl;

	}
}

//对测试图片进行分类

void categorizer::category_By_svm()
{
	cout << "物体分类开始..." << endl;
	Mat gray_pic;
	Mat threshold_image;
	string prediction_category;
	float curConfidence;

	directory_iterator begin_train(TEST_FOLDER);
	directory_iterator end_train;

	for (; begin_train != end_train; ++begin_train)
	{

		//获取该目录下的图片名
		string train_pic_name = (begin_train->path()).filename().string();
		string train_pic_path = string(TEST_FOLDER) + string("/") + (begin_train->path()).filename().string();

		//读取图片
		cout << train_pic_path << endl;
		Mat input_pic = imread(train_pic_path);
		imshow("输入图片：", input_pic);
		cvtColor(input_pic, gray_pic, CV_BGR2GRAY);

		// 提取BOW描述子
		vector<KeyPoint>kp;
		Mat test;
		featureDecter->detect(gray_pic, kp);
		bowDescriptorExtractor->compute(gray_pic, kp, test);

		int sign = 0;
		float best_score = -2.0f;
		try{
			for (int i = 0; i < categories_size; i++)
			{

				string cate_na = category_name[i];

				string f_path = string(DATA_FOLDER) + cate_na + string("SVM.xml");
				FileStorage svm_fs(f_path, FileStorage::READ);
				//读取SVM.xml
				if (svm_fs.isOpened())
				{

					svm_fs.release();
					CvSVM st_svm;
					st_svm.load(f_path.c_str());

					if (sign == 0)
					{
						float score_Value = st_svm.predict(test, true);
						float class_Value = st_svm.predict(test, false);
						sign = (score_Value < 0.0f) == (class_Value < 0.0f) ? 1 : -1;
					}
					curConfidence = sign * st_svm.predict(test, true);
				}
				else
				{
					if (sign == 0)
					{
						float scoreValue = stor_svms[i].predict(test, true);
						float classValue = stor_svms[i].predict(test, false);
						sign = (scoreValue < 0.0f) == (classValue < 0.0f) ? 1 : -1;
					}
					curConfidence = sign * stor_svms[i].predict(test, true);
				}

				if (curConfidence>best_score)
				{
					best_score = curConfidence;
					prediction_category = cate_na;
				}

			}
		}
		catch (exception ex){
			cout << "skip unknow.";
		}
		//将图片写入相应的文件夹下
		directory_iterator begin_iterater(RESULT_FOLDER);
		directory_iterator end_iterator;
		//获取该目录下的文件名
		for (; begin_iterater != end_iterator; ++begin_iterater)
		{

			if (begin_iterater->path().filename().string() == prediction_category)
			{

				string filename = string(RESULT_FOLDER) + prediction_category + string("/") + train_pic_name;
				imwrite(filename, input_pic);
			}

		}
		//显示输出
		//namedWindow("Dectect Object");
		cout << "这张图属于： " << prediction_category << endl;

		//imshow("Dectect Object",result_objects[prediction_category]);
		waitKey(0);
	}
}

int main(void)
{
	int clusters = 1000;
	categorizer c(clusters);
	//特征聚类
	c.bulid_vacab();
	//构造BOW
	c.compute_bow_image();
	//训练分类器
	c.trainSvm();
	//将测试图片分类
	c.category_By_svm();
	return 0;
}