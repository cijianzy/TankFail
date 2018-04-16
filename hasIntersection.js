// 求 2 点之间的距离
function getPointDistance(x1, y1, x2, y2) {
  return Math.sqrt(Math.pow(x2 - x1, 2) + Math.pow(y2 - y1, 2));
}

// 判断圆与线段是否有 2 个交点 || 有一个相切交点
function hasIntersection(x1, y1, x2, y2, x3, y3, r) {
  // 直线公式：Ax + By + C = 0
  // 需要求出 A、B、C
  const A = (y2 - y1) / (x2 - x1);
  const B = -1;
  const C = y1 - A * x1;

  // 圆心到直线的距离
  const lineDistance = Math.abs(
    (A * x3 + B * y3 + C) / Math.sqrt(A * A + B * B)
  );
  // 第一个点到圆心的距离
  const pointOneDistance = getPointDistance(x1, y1, x3, y3);
  // 第二个点到圆心的距离
  const pointTwoDistance = getPointDistance(x2, y2, x3, y3);

  console.log(lineDistance, pointOneDistance, pointTwoDistance)

  if (lineDistance > r) { // 直线与圆外相离，一定没有交点
    return false;
  }

  // 有一个点在圆上，一定有交点
  if (pointOneDistance === r || pointTwoDistance === r) {
    return true;
  }

  if ( // 两个端点，一个在圆内，一个在圆外，一定相交
    Math.min(pointOneDistance, pointTwoDistance) < r &&
    Math.max(pointOneDistance, pointTwoDistance) > r
  ) {
    return true;
  }

  const pointDistanceSelf = getPointDistance(x1, y1, x2, y2); // 2 个点之间的距离
  const beta = Math.acos( // 圆心与 2 个点连线夹角角度「余弦定理」
    (
      pointOneDistance * pointOneDistance +
      pointTwoDistance * pointTwoDistance -
      pointDistanceSelf * pointDistanceSelf
    ) /
    (2 * pointOneDistance * pointTwoDistance)
  );

  if (
    pointOneDistance > r &&
    pointTwoDistance > r &&
    beta > Math.PI / 2
  ) {
    return true;
  }

  // 默认没有交点
  return false;
}
