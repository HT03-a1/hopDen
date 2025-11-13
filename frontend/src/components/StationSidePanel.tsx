import { useState } from 'react';

interface StationSidePanelProps {
  profile: any;
  sosList: any[];
  historySOSList?: any[]; // Lịch sử các SOS đã hoàn thành/hủy
  onClaimSOS: (sosId: string) => void;
  onUpdateStatus: (sosId: string, status: string) => void;
  onToggleReady?: (sosId: string, ready: boolean) => void; // Toggle ready status
  onShowRoute?: (fromLat: number, fromLon: number, toLat: number, toLon: number) => void;
}

// Tính khoảng cách giữa hai điểm (Haversine formula)
function calculateDistance(lat1: number, lon1: number, lat2: number, lon2: number): number {
  const R = 6371; // Bán kính Trái Đất (km)
  const dLat = (lat2 - lat1) * Math.PI / 180;
  const dLon = (lon2 - lon1) * Math.PI / 180;
  const a = 
    Math.sin(dLat / 2) * Math.sin(dLat / 2) +
    Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) *
    Math.sin(dLon / 2) * Math.sin(dLon / 2);
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
  return R * c;
}

export default function StationSidePanel({
  profile,
  sosList,
  historySOSList = [],
  onClaimSOS,
  onUpdateStatus,
  onToggleReady,
  onShowRoute,
}: StationSidePanelProps) {
  const [expandedSection, setExpandedSection] = useState<string | null>('sos');
  
  // Tính khoảng cách từ trạm đến SOS
  const getDistance = (sos: any): number | null => {
    if (!profile?.lat || !profile?.lon || !sos.location?.lat || !sos.location?.lon) {
      return null;
    }
    return calculateDistance(profile.lat, profile.lon, sos.location.lat, sos.location.lon);
  };

  const getStatusColor = (status: string) => {
    switch (status) {
      case 'pending':
        return 'bg-yellow-100 text-yellow-800';
      case 'accepted':
        return 'bg-blue-100 text-blue-800';
      case 'on_route':
        return 'bg-purple-100 text-purple-800';
      case 'done':
        return 'bg-green-100 text-green-800';
      case 'cancelled':
        return 'bg-red-100 text-red-800';
      default:
        return 'bg-gray-100 text-gray-800';
    }
  };

  const getStatusText = (status: string) => {
    switch (status) {
      case 'pending':
        return 'Chờ xử lý';
      case 'accepted':
        return 'Đã nhận';
      case 'on_route':
        return 'Đang đi';
      case 'done':
        return 'Hoàn thành';
      case 'cancelled':
        return 'Đã hủy';
      default:
        return status;
    }
  };

  return (
    <div className="h-full flex flex-col" style={{ 
      backgroundColor: 'rgb(249 250 251)', 
      opacity: 1,
      filter: 'none',
      WebkitFilter: 'none'
    }}>
      <div className="p-3 sm:p-4 bg-gradient-to-r from-slate-700 via-slate-800 to-slate-900 text-white" style={{ 
        opacity: 1,
        filter: 'none',
        WebkitFilter: 'none'
      }}>
        <h2 className="text-base sm:text-lg md:text-xl font-bold truncate">{profile?.stationName}</h2>
        <p className="text-xs sm:text-sm text-slate-200">{profile?.type === 'medical' ? 'Trạm y tế' : 'Trạm cứu hộ'}</p>
      </div>

      <div className="flex-1 overflow-y-auto p-3 sm:p-4 space-y-3 sm:space-y-4">
        {/* Station Info - Responsive */}
        <div className="bg-white rounded-lg shadow p-3 sm:p-4">
          <button
            onClick={() => setExpandedSection(expandedSection === 'info' ? null : 'info')}
            className="w-full flex justify-between items-center font-semibold mb-2 text-sm sm:text-base"
          >
            <span>Thông tin trạm</span>
            <span className="text-xs sm:text-sm">{expandedSection === 'info' ? '▼' : '▶'}</span>
          </button>
          {expandedSection === 'info' && (
            <div className="space-y-2 text-sm">
              <div>
                <span className="font-semibold">ID:</span> {profile.id}
              </div>
              <div>
                <span className="font-semibold">Địa chỉ:</span> {profile.address}
              </div>
              <div>
                <span className="font-semibold">SĐT:</span> {profile.phone}
              </div>
              <div>
                <span className="font-semibold">Giờ làm việc:</span> {profile.openHours}
              </div>
              <div>
                <span className="font-semibold">Đánh giá:</span> ⭐ {profile.ratingAvg?.toFixed(1)} ({profile.ratingCount} lượt)
              </div>
            </div>
          )}
        </div>

        {/* SOS List - Responsive */}
        <div className="bg-white rounded-lg shadow p-3 sm:p-4">
          <button
            onClick={() => setExpandedSection(expandedSection === 'sos' ? null : 'sos')}
            className="w-full flex justify-between items-center font-semibold mb-2 text-sm sm:text-base"
          >
            <span>SOS trong khu vực ({sosList.length})</span>
            <span className="text-xs sm:text-sm">{expandedSection === 'sos' ? '▼' : '▶'}</span>
          </button>
          {expandedSection === 'sos' && (
            <div className="space-y-2 sm:space-y-3 max-h-96 overflow-y-auto">
              {sosList.length === 0 ? (
                <p className="text-xs sm:text-sm text-gray-500">Không có SOS nào</p>
              ) : (
                sosList.map((sos) => (
                  <div key={sos.id} className="border rounded-lg p-2 sm:p-3 bg-gray-50">
                    <div className="flex justify-between items-start mb-2">
                      <div>
                        <div className="font-semibold">
                          {sos.type === 'accident' && '🚗 Tai nạn'}
                          {sos.type === 'breakdown' && '🔧 Hỏng xe'}
                          {sos.type === 'medical' && '🏥 Y tế'}
                          {sos.type === 'other' && '⚠️ Khác'}
                        </div>
                        <div className="text-xs text-gray-600">
                          Mức độ: {sos.severity}
                        </div>
                      </div>
                      <span className={`text-[10px] sm:text-xs px-1.5 sm:px-2 py-0.5 sm:py-1 rounded ${getStatusColor(sos.status)}`}>
                        {getStatusText(sos.status)}
                      </span>
                    </div>
                    {sos.note && (
                      <div className="text-sm text-gray-700 mb-2">{sos.note}</div>
                    )}
                    <div className="text-xs text-gray-500 mb-2">
                      {new Date(sos.createdAt).toLocaleString('vi-VN')}
                    </div>
                    {/* Hiển thị quãng đường - chỉ hiển thị khi SOS chưa done để bảo vệ quyền riêng tư */}
                    {sos.status !== 'done' && sos.status !== 'cancelled' && profile?.lat && profile?.lon && sos.location && (() => {
                      const distance = getDistance(sos);
                      return distance !== null ? (
                        <div className="text-xs text-blue-600 font-semibold mb-2">
                          📍 Khoảng cách: {distance.toFixed(2)} km
                        </div>
                      ) : null;
                    })()}
                    {/* Ẩn thông tin vị trí khi SOS đã done để bảo vệ quyền riêng tư */}
                    {(sos.status === 'done' || sos.status === 'cancelled') && (
                      <div className="text-xs text-gray-500 italic mb-2">
                        🔒 Thông tin vị trí đã được ẩn để bảo vệ quyền riêng tư
                      </div>
                    )}
                    <div className="flex flex-wrap gap-2">
                      {sos.status === 'pending' && (
                        <>
                          {/* Nếu SOS đã được gán cho trạm này, hiển thị cả nút Nhận và Không nhận */}
                          {sos.assignedStationId === profile.id ? (
                            <>
                              <button
                                onClick={() => onClaimSOS(sos.id)}
                                className="text-[10px] sm:text-xs bg-green-500 text-white px-2 sm:px-3 py-1 rounded hover:bg-green-600 transition"
                              >
                                ✅ Nhận nhiệm vụ
                              </button>
                              <button
                                onClick={() => {
                                  if (window.confirm('Bạn có chắc chắn không nhận nhiệm vụ này? Hệ thống sẽ tự động chuyển sang trạm khác.')) {
                                    onUpdateStatus(sos.id, 'cancelled');
                                  }
                                }}
                                className="text-[10px] sm:text-xs bg-red-500 text-white px-2 sm:px-3 py-1 rounded hover:bg-red-600 transition"
                              >
                                ❌ Không nhận nhiệm vụ
                              </button>
                            </>
                          ) : (
                            /* Nếu SOS chưa được gán cho trạm này, hiển thị nút "Sẵn sàng nhận nhiệm vụ" */
                            (() => {
                              const isReady = sos.readyStationIds?.includes(profile.id) || false;
                              return (
                                <button
                                  onClick={() => {
                                    if (onToggleReady) {
                                      onToggleReady(sos.id, !isReady);
                                    }
                                  }}
                                  className={`text-[10px] sm:text-xs px-2 sm:px-3 py-1 rounded transition ${
                                    isReady 
                                      ? 'bg-yellow-500 text-white hover:bg-yellow-600' 
                                      : 'bg-green-500 text-white hover:bg-green-600'
                                  }`}
                                >
                                  <span className="truncate">{isReady ? '🟡 Sẵn sàng nhận nhiệm vụ' : '🟢 Nhận nhiệm vụ'}</span>
                                </button>
                              );
                            })()
                          )}
                          {/* Chỉ hiển thị nút chỉ đường khi SOS chưa done để bảo vệ quyền riêng tư */}
                          {sos.status !== 'done' && sos.status !== 'cancelled' && onShowRoute && profile?.lat && profile?.lon && sos.location && (
                            <button
                              onClick={() => {
                                const fromLat = parseFloat(profile.lat);
                                const fromLon = parseFloat(profile.lon);
                                const toLat = parseFloat(sos.location.lat);
                                const toLon = parseFloat(sos.location.lon);
                                console.log('Showing route:', { 
                                  from: { lat: fromLat, lon: fromLon }, 
                                  to: { lat: toLat, lon: toLon },
                                  sosId: sos.id
                                });
                                onShowRoute(fromLat, fromLon, toLat, toLon);
                              }}
                              className="text-[10px] sm:text-xs bg-blue-500 text-white px-2 sm:px-3 py-1 rounded hover:bg-blue-600 transition"
                            >
                              🗺️ Chỉ đường
                            </button>
                          )}
                        </>
                      )}
                      {sos.status === 'accepted' && sos.assignedStationId === profile.id && (
                        <>
                          <button
                            onClick={() => onUpdateStatus(sos.id, 'on_route')}
                            className="text-[10px] sm:text-xs bg-blue-500 text-white px-2 sm:px-3 py-1 rounded hover:bg-blue-600 transition"
                          >
                            Bắt đầu đi
                          </button>
                          {/* Chỉ hiển thị nút chỉ đường khi SOS chưa done để bảo vệ quyền riêng tư */}
                          {sos.status !== 'done' && sos.status !== 'cancelled' && onShowRoute && profile?.lat && profile?.lon && sos.location && (
                            <button
                              onClick={() => {
                                const fromLat = parseFloat(profile.lat);
                                const fromLon = parseFloat(profile.lon);
                                const toLat = parseFloat(sos.location.lat);
                                const toLon = parseFloat(sos.location.lon);
                                console.log('Showing route:', { 
                                  from: { lat: fromLat, lon: fromLon }, 
                                  to: { lat: toLat, lon: toLon },
                                  sosId: sos.id
                                });
                                onShowRoute(fromLat, fromLon, toLat, toLon);
                              }}
                              className="text-[10px] sm:text-xs bg-purple-500 text-white px-2 sm:px-3 py-1 rounded hover:bg-purple-600 transition"
                            >
                              🗺️ Chỉ đường
                            </button>
                          )}
                          {/* Chỉ hiển thị nút Mở Google Maps khi SOS chưa done để bảo vệ quyền riêng tư */}
                          {sos.status !== 'done' && sos.status !== 'cancelled' && profile?.lat && profile?.lon && sos.location && (
                            <button
                              onClick={() => {
                                // Open Google Maps with route
                                const fromLat = parseFloat(profile.lat);
                                const fromLon = parseFloat(profile.lon);
                                const toLat = parseFloat(sos.location.lat);
                                const toLon = parseFloat(sos.location.lon);
                                
                                const googleMapsUrl = `https://www.google.com/maps/dir/${fromLat},${fromLon}/${toLat},${toLon}`;
                                window.open(googleMapsUrl, '_blank');
                              }}
                              className="text-[10px] sm:text-xs bg-orange-500 text-white px-2 sm:px-3 py-1 rounded hover:bg-orange-600 transition"
                            >
                              Mở Google Maps
                            </button>
                          )}
                          <button
                            onClick={() => {
                              if (window.confirm('Bạn có chắc chắn muốn hủy nhiệm vụ này?')) {
                                onUpdateStatus(sos.id, 'cancelled');
                              }
                            }}
                            className="text-[10px] sm:text-xs bg-red-500 text-white px-2 sm:px-3 py-1 rounded hover:bg-red-600 transition"
                          >
                            Hủy nhiệm vụ
                          </button>
                        </>
                      )}
                      {sos.status === 'on_route' && sos.assignedStationId === profile.id && (
                        <>
                          <button
                            onClick={() => {
                              if (window.confirm('Xác nhận hoàn thành nhiệm vụ này?')) {
                                onUpdateStatus(sos.id, 'done');
                              }
                            }}
                            className="text-[10px] sm:text-xs bg-green-500 text-white px-2 sm:px-3 py-1 rounded hover:bg-green-600 transition font-semibold"
                          >
                            ✓ Hoàn thành nhiệm vụ
                          </button>
                          {/* Chỉ hiển thị nút chỉ đường khi SOS chưa done để bảo vệ quyền riêng tư */}
                          {sos.status !== 'done' && sos.status !== 'cancelled' && onShowRoute && profile?.lat && profile?.lon && sos.location && (
                            <button
                              onClick={() => {
                                const fromLat = parseFloat(profile.lat);
                                const fromLon = parseFloat(profile.lon);
                                const toLat = parseFloat(sos.location.lat);
                                const toLon = parseFloat(sos.location.lon);
                                console.log('Showing route:', { 
                                  from: { lat: fromLat, lon: fromLon }, 
                                  to: { lat: toLat, lon: toLon },
                                  sosId: sos.id
                                });
                                onShowRoute(fromLat, fromLon, toLat, toLon);
                              }}
                              className="text-[10px] sm:text-xs bg-purple-500 text-white px-2 sm:px-3 py-1 rounded hover:bg-purple-600 transition"
                            >
                              🗺️ Chỉ đường
                            </button>
                          )}
                          <button
                            onClick={() => {
                              if (window.confirm('Bạn có chắc chắn muốn hủy nhiệm vụ này?')) {
                                onUpdateStatus(sos.id, 'cancelled');
                              }
                            }}
                            className="text-[10px] sm:text-xs bg-red-500 text-white px-2 sm:px-3 py-1 rounded hover:bg-red-600 transition"
                          >
                            Hủy nhiệm vụ
                          </button>
                        </>
                      )}
                    </div>
                  </div>
                ))
              )}
            </div>
          )}
        </div>

        {/* Lịch sử */}
        <div className="bg-white rounded-lg shadow p-4">
          <button
            onClick={() => setExpandedSection(expandedSection === 'history' ? null : 'history')}
            className="w-full flex justify-between items-center font-semibold mb-2"
          >
            <span>Lịch sử ({historySOSList.length})</span>
            <span>{expandedSection === 'history' ? '▼' : '▶'}</span>
          </button>
          {expandedSection === 'history' && (
            <div className="space-y-3 max-h-96 overflow-y-auto">
              {historySOSList.length === 0 ? (
                <p className="text-sm text-gray-500">Chưa có lịch sử</p>
              ) : (
                historySOSList
                  .sort((a, b) => {
                    // Sắp xếp theo thời gian cập nhật mới nhất trước
                    const dateA = new Date(a.updatedAt || a.createdAt).getTime();
                    const dateB = new Date(b.updatedAt || b.createdAt).getTime();
                    return dateB - dateA;
                  })
                  .map((sos) => (
                    <div key={sos.id} className="border rounded-lg p-3 bg-gray-50">
                      <div className="flex justify-between items-start mb-2">
                        <div>
                          <div className="font-semibold">
                            {sos.type === 'accident' && '🚗 Tai nạn'}
                            {sos.type === 'breakdown' && '🔧 Hỏng xe'}
                            {sos.type === 'medical' && '🏥 Y tế'}
                            {sos.type === 'other' && '⚠️ Khác'}
                          </div>
                          <div className="text-xs text-gray-600">
                            Mức độ: {sos.severity}
                          </div>
                        </div>
                        <span className={`text-xs px-2 py-1 rounded ${getStatusColor(sos.status)}`}>
                          {getStatusText(sos.status)}
                        </span>
                      </div>
                      {sos.note && (
                        <div className="text-sm text-gray-700 mb-2">{sos.note}</div>
                      )}
                      <div className="text-xs text-gray-500 mb-2">
                        <div>Thời gian tạo: {new Date(sos.createdAt).toLocaleString('vi-VN')}</div>
                        {sos.updatedAt && sos.updatedAt !== sos.createdAt && (
                          <div>Thời gian cập nhật: {new Date(sos.updatedAt).toLocaleString('vi-VN')}</div>
                        )}
                      </div>
                      {/* Luôn ẩn thông tin vị trí trong lịch sử để bảo vệ quyền riêng tư */}
                      <div className="text-xs text-gray-500 italic mb-2">
                        🔒 Thông tin vị trí đã được ẩn để bảo vệ quyền riêng tư
                      </div>
                    </div>
                  ))
              )}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

