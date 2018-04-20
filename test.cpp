// basic file operations
#include <iostream>
#include <fstream>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <chrono>

using namespace std;
# include <iostream>
# include <chrono>
using namespace std;
using  ns = chrono::nanoseconds;
using get_time = chrono::steady_clock ;
int answer_to_everything()
{
    return 42;
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
inline float getDistance(float x1, float y1, float x2, float y2) {
    return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}
double safeAcos(double x)
{
    if (x < -1.0) x = -1.0 ;
    else if (x > 1.0) x = 1.0 ;
    return acos (x) ;
}
int main()
{

    double x = 0 , y = 0 , x1 = -1, y1 = 1, direction = 7.0/8 * (3.14 * 2);

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

    // TODO_CIJIAN  2018 ,Apr18 , Wed, 23:03
    angleAns = safeAcos((b * b + a * a - c * c)/ (2 * a* b));

    double tempAngle = atan2(y1 - y, x1 - x);
    if (angleIn2PI(direction - tempAngle) > 3.14) {
        angleAns = atan2(y1 - y, x1 - x) - angleAns;
    } else {
        angleAns = atan2(y1 - y, x1 - x) + angleAns;
    }
    angleAns = angleIn2PI(angleAns);

    cout << angleAns << endl;

}
