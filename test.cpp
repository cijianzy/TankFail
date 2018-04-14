// basic file operations
#include <iostream>
#include <fstream>
#include <math.h>
using namespace std;

int writeFile ()
{
//    ofstream myfile;
//    myfile.open ("example.txt");
//    myfile << "Writing this to a file.\n";
//    myfile << "Writing this to a file.\n";
//    myfile << "Writing this to a file.\n";
//    myfile << "Writing this to a file.\n";
//    myfile.close();
//
//    system("cd ../tank_ai");
//    system("pwd");
//    system("bash ../tank_ai/new_game.sh");
    return 0;
}

inline float getDistance(float x1, float y1, float x2, float y2) {
    return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

inline double xyAttackAngle(double x, double y, double x1, double y1, double direction) {

    // 追击公式  91 * x * x = y * y - 6*x*y*cos(θ)
    double angle = M_PI + (atan2(y1 - y, x1 - x) - direction); // 相差角度
    double distance = getDistance(x1, y1, x, y);

    double a = -91;
    double cosAngle = cos(angle);
    double b = -6 * distance * cosAngle;
    double c = distance * distance;
    int temp = b * b - 4 * a * c;
    double xAns;
    if (temp < 0){
    } else if (temp == 0) {
        xAns = -1 * (b * 1.0) / (2 * a);
    } else {
        double x1 = (-1 * b + sqrt(temp))/(2 * a);
        double x2  = (-1 * b - sqrt(temp))/(2 * a);

        if(x1>0) {
            xAns = x1;
        } else {
            xAns = x2;
        }
    }

    a = 10 * xAns;
    b = distance;
    c = 3 * xAns;

    double angleAns = acos((b * b + a * a - c * c)/ (2 * a* b));

    if (direction <= 3.14) {
        angleAns += atan2(y1 - y, x1 - x);
    } else {
        angleAns -= atan2(y1 - y, x1 - x);
    }
    angleAns =  + angleAns;

    while(angleAns > 2 * M_PI) {
        angleAns -= 2 * M_PI;
    }

    while(angleAns < 0) {
        angleAns += 2 * M_PI;
    }

    return angleAns;

}

int main() {
//    cout << cos(3.14) << endl;

    xyAttackAngle(2.03097, 7.88417, 10.507, 0, 5.09636);
}
