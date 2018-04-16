// basic file operations
#include <iostream>
#include <fstream>
#include <math.h>
using namespace std;

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
//    return 0;
//}

inline float getDistance(float x1, float y1, float x2, float y2) {
    return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}


inline double angleIn2PI(double angle) {
    while (angle > 2 * M_PI) {
        angle -= 2 * M_PI;
    }

    while(angle < 0) {
        angle += M_PI * 2;
    }

    return angle;
}

inline double xyAttackAngle(double x, double y, double x1, double y1, double direction) {

    // 追击公式  91 * x * x = y * y - 6*x*y*cos(θ)
    y = -y;
    y1 = -y1;

    cout << atan2(y - y1, x - x1) << endl;
    double angle = angleIn2PI(atan2(y - y1, x - x1)) - direction; // 相差角度
    angle = angleIn2PI(angle);
    if (angle > M_PI) {
        angle = M_2_PI - angle;
    }
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
    double angleAns = 0;
    if (abs(a+c-b) < 0.001) {
        angleAns = 0;
    } else {
        angleAns = acos((b * b + a * a - c * c)/ (2 * a* b));
        double tempAngle = atan2(y1 - y, x1 - x);
        if (angleIn2PI(direction - tempAngle) > 3.14) {
            angleAns = atan2(y1 - y, x1 - x) - angleAns;
        } else {
            angleAns = atan2(y1 - y, x1 - x) + angleAns;
        }
    }

    angleAns = angleIn2PI(angleAns);

    return angleAns;

}

int main() {
//    cout << cos(3.14) << endl;
    xyAttackAngle( 5.85472 ,  8.8623, 0 , 1.14909, 3.75246);
}
//attack func myPosition: 11.5444 6.9601targetId ai:132targetPosition: 3.68668 15 targetDirection: 0.994838 result: 3.71427
