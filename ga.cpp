#include <cstdio>
#include <QtGui>

#define IMGSIZE 256
#define NGON 4
#define POLYGONS 50
#define POPULATION 10

uint distance(QImage &img1, QImage &img2);
void calcDistance(uint* distances);
QImage sourceImg;
double*** population;

uint distance(QImage &img1, QImage &img2)
{
	if (img1.numBytes() != img2.numBytes())
		printf("Image Size Mismatch!\n");
	int dist = 0;
	const uchar* bytes1 = img1.bits();
	const uchar* bytes2 = img2.bits();
	for (int i = 0; i < img1.numBytes(); i++) {
		uint dbyte = bytes1[i]-bytes2[i];
		dist      += dbyte * dbyte;
	}
	return dist;
}
//data format, [r,g,b,a,x0,y0,x1,y1,xn,yn]
//all go from 0.0 to 1.0 to ease computation

//calculate the distance from the source image for each member of the
//population storing it in an array (of population size) passed in
void calcDistance(int* distances)
{
	for (int p = 0; p < POPULATION; p++) {
		//draw polygons on image
		QImage drawImage(IMGSIZE,IMGSIZE,QImage::Format_RGB32);
		QPainter painter(&drawImage);
		for (int n = 0; n < POLYGONS; n++) {
			double* polygon = population[p][n];
			painter.setPen(QColor(polygon[0]*255,polygon[1]*255,polygon[2]*255,polygon[3]*255));
			int points[NGON*2];
			for (int i = 0; i < NGON*2; i++)
				points[i] = polygon[4+i]*(IMGSIZE-1);
			QPolygon poly;
			poly.setPoints(NGON,points);
			painter.drawPolygon(poly);
		}
		painter.end();
		//distance is distance in 3-space of the colors
		int numbytes = drawImage.numBytes();
		int dist = 0;
		uchar* simg = sourceImg.bits();
		uchar* dimg = drawImage.bits();
		for (int n = 0; n < numbytes; n++) {
			if (n % 4 == 0) //TODO: make sure this works to skip alpha channel
				continue;
			int diff = simg[n]-dimg[n];
			dist += diff*diff;
		}
		distances[p] = dist;
	}
}

int main(int argc, char* argv[])
{
	if (argc > 1)
		sourceImg = QImage(argv[1]).scaled(IMGSIZE,IMGSIZE);
	else {
		printf("You must specify the image to generate as second option\n");
		exit(1);
	}
	population = new double**[POPULATION];
	for (int p = 0; p < POPULATION; p++) {
		population[p] = new double*[POLYGONS];
		for (int i = 0; i < POLYGONS; i++) {
			population[p][i] = new double[4+2*NGON];
			for (int j = 0; j < 4+2*NGON; j++)
				population[p][i][j] = (double)rand()/RAND_MAX;
			population[p][i][3] = 1.0; //FIXME: start opaque for testing
		}
	}
}
