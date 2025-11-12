import { useState } from 'react';

interface StationSidePanelProps {
  profile: any;
  sosList: any[];
  onClaimSOS: (sosId: string) => void;
  onUpdateStatus: (sosId: string, status: string) => void;
}

export default function StationSidePanel({
  profile,
  sosList,
  onClaimSOS,
  onUpdateStatus,
}: StationSidePanelProps) {
  const [expandedSection, setExpandedSection] = useState<string | null>('sos');

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
      default:
        return status;
    }
  };

  return (
    <div className="h-full flex flex-col">
      <div className="p-4 bg-blue-600 text-white">
        <h2 className="text-xl font-bold">{profile?.stationName}</h2>
        <p className="text-sm text-blue-100">{profile?.type === 'medical' ? 'Trạm y tế' : 'Trạm cứu hộ'}</p>
      </div>

      <div className="flex-1 overflow-y-auto p-4 space-y-4">
        {/* Station Info */}
        <div className="bg-white rounded-lg shadow p-4">
          <button
            onClick={() => setExpandedSection(expandedSection === 'info' ? null : 'info')}
            className="w-full flex justify-between items-center font-semibold mb-2"
          >
            <span>Thông tin trạm</span>
            <span>{expandedSection === 'info' ? '▼' : '▶'}</span>
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

        {/* SOS List */}
        <div className="bg-white rounded-lg shadow p-4">
          <button
            onClick={() => setExpandedSection(expandedSection === 'sos' ? null : 'sos')}
            className="w-full flex justify-between items-center font-semibold mb-2"
          >
            <span>SOS trong khu vực ({sosList.length})</span>
            <span>{expandedSection === 'sos' ? '▼' : '▶'}</span>
          </button>
          {expandedSection === 'sos' && (
            <div className="space-y-3 max-h-96 overflow-y-auto">
              {sosList.length === 0 ? (
                <p className="text-sm text-gray-500">Không có SOS nào</p>
              ) : (
                sosList.map((sos) => (
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
                      {new Date(sos.createdAt).toLocaleString('vi-VN')}
                    </div>
                    <div className="flex flex-wrap gap-2">
                      {sos.status === 'pending' && (
                        <button
                          onClick={() => onClaimSOS(sos.id)}
                          className="text-xs bg-green-500 text-white px-3 py-1 rounded hover:bg-green-600"
                        >
                          Nhận nhiệm vụ
                        </button>
                      )}
                      {sos.status === 'accepted' && sos.assignedStationId === profile.id && (
                        <>
                          <button
                            onClick={() => onUpdateStatus(sos.id, 'on_route')}
                            className="text-xs bg-blue-500 text-white px-3 py-1 rounded hover:bg-blue-600"
                          >
                            Bắt đầu đi
                          </button>
                          <button
                            onClick={() => {
                              // Open Google Maps with route
                              const fromLat = profile.lat;
                              const fromLon = profile.lon;
                              const toLat = sos.location.lat;
                              const toLon = sos.location.lon;
                              
                              const googleMapsUrl = `https://www.google.com/maps/dir/${fromLat},${fromLon}/${toLat},${toLon}`;
                              window.open(googleMapsUrl, '_blank');
                            }}
                            className="text-xs bg-purple-500 text-white px-3 py-1 rounded hover:bg-purple-600"
                          >
                            Mở Google Maps
                          </button>
                        </>
                      )}
                      {sos.status === 'on_route' && sos.assignedStationId === profile.id && (
                        <button
                          onClick={() => onUpdateStatus(sos.id, 'done')}
                          className="text-xs bg-green-500 text-white px-3 py-1 rounded hover:bg-green-600"
                        >
                          Hoàn thành
                        </button>
                      )}
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

