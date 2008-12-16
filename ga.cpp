#include <cstdio>
#include <vector>
#include <QtGui>
#include <QSvgGenerator>
#include <unistd.h>

#define NGON 4
#define POLYGONS 50
#define POPULATION 12
#define SIZEOFPOLY (4+2*NGON)
#define TOKILL 6

void calcDistance(long long* distances);
QImage drawImage(double** member);
void drawSvg(double** member, const char* svgname);
void mutate(double** member);
std::vector<double**> population;
double** breed(double** parent1, double** parent2);
long long distance(QImage img);
void initga();
void gastep();
void runga();

QImage sourceImg; //image to match
int imgw,imgh; //dimensions of image
int gens = -1, fitness = -1; //fitness and generation limit

//given a population member, randomly mutate one aspect of it,
//either by changing it a small amount or completely changing it
void mutate(double** member)
{
	int poly  = rand()%POLYGONS;
	int field = rand()%(4+2*NGON);
	if (rand()%2) {
		member[poly][field] = (double)rand()/RAND_MAX;
	} else {
		member[poly][field] += (double)rand()/RAND_MAX*0.2 - 0.1;
		if (member[poly][field] < 0.0)
			member[poly][field] = 0.0;
		if (member[poly][field] > 1.0)
			member[poly][field] = 1.0;
	}
}

//generate a hybrid of two members using the probably poor
//scheme of just randomly choosing polygons
double** breed(double** parent1, double** parent2)
{
	double** child = new double*[POLYGONS];
	#pragma omp parallel for
	for (int p = 0; p < POLYGONS; p++) {
		child[p] = new double[SIZEOFPOLY];
		int r = rand()&1;
		if (r) {
			memcpy(child[p],parent1[p],SIZEOFPOLY*sizeof(double));
		} else {
			memcpy(child[p],parent2[p],SIZEOFPOLY*sizeof(double));
		}
	}
	return child;
}

//data format, [r,g,b,a,x0,y0,x1,y1,xn,yn]
//all go from 0.0 to 1.0 to ease computation

//rasterize a member and return it
QImage drawImage(double** member)
{
		QImage drawImage(imgw,imgh,QImage::Format_RGB32);
		QPainter painter(&drawImage);
		painter.setPen(Qt::NoPen);
		for (int n = 0; n < POLYGONS; n++) {
			double* polygon = member[n];
			painter.setBrush(QBrush(QColor(polygon[0]*255,polygon[1]*255,polygon[2]*255,polygon[3]*255)));
			int points[NGON*2];
			for (int i = 0; i < NGON*2; i++)
				points[i] = polygon[4+i]*(i?imgh:imgw);
			QPolygon poly;
			poly.setPoints(NGON,points);
			painter.drawPolygon(poly);
		}
		painter.end();
		return drawImage;
}

//convert member to svg and write it with filename
void drawSvg(double** member, const char* svgname)
{
		QSvgGenerator svggen;
		svggen.setFileName(QString(svgname));
		svggen.setSize(QSize(imgw,imgh));
		QPainter painter(&svggen);
		painter.setPen(Qt::NoPen);
		for (int n = 0; n < POLYGONS; n++) {
			double* polygon = member[n];
			painter.setBrush(QBrush(QColor(polygon[0]*255,polygon[1]*255,polygon[2]*255,polygon[3]*255)));
			int points[NGON*2];
			for (int i = 0; i < NGON*2; i++)
				points[i] = polygon[4+i]*(i?imgh:imgw);
			QPolygon poly;
			poly.setPoints(NGON,points);
			painter.drawPolygon(poly);
		}
		painter.end();
}

//calculate the distance from the source image
long long distance(QImage img)
{
		//distance is distance in 3-space of the colors
		//TODO:maybe use Delta-E? http://www.colorwiki.com/wiki/Delta_E:_The_Color_Difference
		int numbytes = img.numBytes();
		long long dist = 0;
		uchar* simg = sourceImg.bits();
		uchar* dimg = img.bits();
		for (int n = 0; n < numbytes; n++) {
			int diff = simg[n]-dimg[n];
			dist += diff*diff;
		}
		return dist;
}

//calculate the distance from the source image for each member of the
//population storing it in an array (of population size) passed in
void calcDistance(long long* distances)
{
	#pragma omp parallel for
	for (int p = 0; p < (int)population.size(); p++) {
		//draw polygons on image
		QImage drawImg = drawImage(population[p]);
		distances[p] = distance(drawImg);
	}
}

//initialize the population of the genetic algorithm
void initga()
{
	for (int p = 0; p < POPULATION; p++) {
		population.push_back(new double*[POLYGONS]); //TODO: presize vector?
		for (int i = 0; i < POLYGONS; i++) {
			population[p][i] = new double[4+2*NGON];
			for (int j = 0; j < 4+2*NGON; j++)
				population[p][i][j] = 0.0;//(double)rand()/RAND_MAX;
		}
	}
}

//run an iteration of the genetic algorithm
void gastep() {
	long long* distances = new long long[population.size()];
	calcDistance(distances);
	//FIXME: use faster sort than selection
	{
		for (int n = 1; n < population.size(); n++) {
			int min_i = n-1;
			for (int m = n; m < population.size(); m++)
				if (distances[m] < distances[min_i])
					min_i = m;
			if (n-1 != min_i) {
				double** tmp_p = population[n-1];
				long long tmp_d = distances[n-1];
				population[n-1] = population[min_i];
				distances[n-1] = distances[min_i];
				population[min_i] = tmp_p;
				distances[min_i] = tmp_d;
			}	
		}
	}
	for (int i = 0; i < TOKILL; i++) {
		for (int p = 0; p < POLYGONS; p++)
			delete population[population.size()-1][p];
		delete population[population.size()-1];
		population.pop_back();
	}
	//breed new ones
	//FIXME: use better breeding strategy than this one
	for (int n = 1; n <= TOKILL; n++) {
		population.push_back(breed(population[0],population[n]));
	}
	//mutate them all
	#pragma omp parallel for
	for (int n = 0; n < population.size(); n++)
		mutate(population[n]);
	delete distances;
}

//command passes to this to do the running of the genetic algorithm
void runga()
{
	int gen = 0;
	while (gens >= 0 && gen <= gens) {
		if (gen%100==0)
			printf("generation %d\n",gen);
		gastep();
		if (!(gen%1000)) {
			QImage a = drawImage(population[0]);
			a.save(QString("out") + QString::number(gen) + QString(".png"));
			printf("fitness: %lld\n",distance(a));
		}
		gen++;
	}
	drawSvg(population[0],"output.svg");
}

int main(int argc, char* argv[])
{
	int opt;
	while ((opt = getopt(argc,argv, "g:")) != -1) {
		switch (opt) {
			//generation limit
			case 'g':
				gens = atoi(optarg);
				if (gens == 0) {
					fprintf(stderr, "You must specify a generation limit with -g, for example -g 100\n");
					exit(EXIT_FAILURE);
				} else if (gens < 0) {
					fprintf(stderr, "You must specify a postive number of generations\n");
					exit(EXIT_FAILURE);
				}
					fprintf(stderr, "Running with %d generations\n", gens);
				break;
		}
	}
	if (optind >= argc) {
		fprintf(stderr, "Expected source image after options\n");
		exit(EXIT_FAILURE);
	}
	FILE* image = fopen(argv[optind],"r");
	if (!image) {
		fprintf(stderr, "Image %s does not exist\n", argv[optind]);
		exit(EXIT_FAILURE);
	}
	fclose(image);

	sourceImg = QImage(argv[optind]);
	imgw = sourceImg.width();
	imgh = sourceImg.height();
	initga();
	runga();
}
