/**
 * Utility: Tính vận tốc từ 2 điểm GPS
 * 
 * Hàm tiện ích để tính khoảng cách và vận tốc giữa 2 điểm GPS
 * Sử dụng công thức Haversine để tính khoảng cách trên mặt cầu
 */

/**
 * Tính khoảng cách giữa 2 điểm GPS (Haversine formula)
 * @param lat1 Vĩ độ điểm 1 (độ)
 * @param lon1 Kinh độ điểm 1 (độ)
 * @param lat2 Vĩ độ điểm 2 (độ)
 * @param lon2 Kinh độ điểm 2 (độ)
 * @returns Khoảng cách tính bằng mét (m)
 */
export function calculateDistance(lat1: number, lon1: number, lat2: number, lon2: number): number {
  // Bán kính Trái Đất (mét)
  const R = 6371000; // 6371 km = 6371000 m
  
  // Chuyển độ sang radian
  const dLat = (lat2 - lat1) * Math.PI / 180;
  const dLon = (lon2 - lon1) * Math.PI / 180;
  
  const a = Math.sin(dLat / 2) * Math.sin(dLat / 2) +
            Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) *
            Math.sin(dLon / 2) * Math.sin(dLon / 2);
  
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
  const distance = R * c;
  
  return distance;
}

/**
 * Tính vận tốc từ 2 điểm GPS và thời gian
 * @param lat1 Vĩ độ điểm 1 (độ)
 * @param lon1 Kinh độ điểm 1 (độ)
 * @param time1 Thời gian điểm 1 (Date hoặc timestamp milliseconds)
 * @param lat2 Vĩ độ điểm 2 (độ)
 * @param lon2 Kinh độ điểm 2 (độ)
 * @param time2 Thời gian điểm 2 (Date hoặc timestamp milliseconds)
 * @returns Vận tốc tính bằng km/h (0 nếu thời gian <= 0)
 */
export function calculateSpeed(
  lat1: number,
  lon1: number,
  time1: Date | number,
  lat2: number,
  lon2: number,
  time2: Date | number
): number {
  // Chuyển Date thành timestamp nếu cần
  const timestamp1 = time1 instanceof Date ? time1.getTime() : time1;
  const timestamp2 = time2 instanceof Date ? time2.getTime() : time2;
  
  // Kiểm tra thời gian hợp lệ
  if (timestamp2 <= timestamp1) {
    return 0; // Không di chuyển hoặc thời gian không hợp lệ
  }
  
  // Tính khoảng cách (mét)
  const distance = calculateDistance(lat1, lon1, lat2, lon2);
  
  // Tính thời gian (giây)
  const timeSeconds = (timestamp2 - timestamp1) / 1000;
  
  if (timeSeconds <= 0) {
    return 0;
  }
  
  // Tính vận tốc (m/s)
  const speedMs = distance / timeSeconds;
  
  // Chuyển sang km/h
  const speedKmh = speedMs * 3.6;
  
  return speedKmh;
}

/**
 * Tính vận tốc từ khoảng cách và thời gian (đã biết)
 * @param distance Khoảng cách (mét)
 * @param timeMs Thời gian (milliseconds)
 * @returns Vận tốc tính bằng km/h
 */
export function calculateSpeedFromDistance(distance: number, timeMs: number): number {
  if (timeMs <= 0) {
    return 0;
  }
  
  const timeSeconds = timeMs / 1000;
  const speedMs = distance / timeSeconds;
  const speedKmh = speedMs * 3.6;
  
  return speedKmh;
}

/**
 * Ví dụ sử dụng:
 * 
 * // Tính vận tốc từ 2 điểm GPS
 * const speed = calculateSpeed(
 *   21.0285, 105.8542, new Date('2025-01-13T10:00:00Z'),
 *   21.0300, 105.8560, new Date('2025-01-13T10:00:30Z')
 * );
 * console.log(`Speed: ${speed.toFixed(2)} km/h`);
 * 
 * // Tính khoảng cách
 * const distance = calculateDistance(21.0285, 105.8542, 21.0300, 105.8560);
 * console.log(`Distance: ${distance.toFixed(2)} m`);
 */

