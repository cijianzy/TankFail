// 求2个点之间的距离
function pointDistance(x1, y1, x2, y2) {
  return Math.sqrt(Math.pow(x2 - x1) + Math.pow(y2 - y1));
}

// 判断圆与线段是否有 2 个交点，或者 一个相切交点
function hasIntersection(x1, y1, x2, y2, x3, y3, r) {
  // 直线公司  Ax + By + C = 0
  // 需要求出 A、B、C
  const B = (y1 * x2 / x1 - y2) / (x2 / x1 - 1);
  const A = (y1 - B) / x1;
  const C = -1;

  // 圆心到直线的距离
  const linedistance = Math.abs(
    (A * x3 + B * y3 + C) / Math.sqrt(A * A + B * B)
  );
  // 第一点到圆心的距离
  const pointOneDistance = pointDistance(x1, y1, x3, y3);
  // 第二点到圆心的距离
  const pointTwoDistance = pointDistance(x1, y1, x3, y3);

  if ( // 直线和园有交点 && 二个点都在园外
    linedistance < r &&
    pointOneDistance > r &&
    pointTwoDistance > r
  ) {
    return true;
  }

  // 默认没有交点
  return false;
}
