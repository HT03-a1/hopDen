import { useState, useEffect } from 'react';

interface UserSidePanelProps {
  profile: any;
  sosList: any[];
  onSendSOS: () => void;
  onFindNearestStation: (type: 'medical' | 'rescue') => void;
  onShowRoute: (fromLat: number, fromLon: number, toLat: number, toLon: number) => void;
  onRateStation: (station: any) => void;
  showSOSModal: boolean;
}

export default function UserSidePanel({
  profile,
  sosList,
  onSendSOS,
  onFindNearestStation,
  onShowRoute,
  onRateStation,
  showSOSModal,
}: UserSidePanelProps) {
  const [expandedSection, setExpandedSection] = useState<string | null>('info');
  const [isBlinking, setIsBlinking] = useState(false);

  // Blinking effect for SOS button when modal is open
  useEffect(() => {
    if (showSOSModal) {
      const interval = setInterval(() => {
        setIsBlinking(prev => !prev);
      }, 500);
      return () => clearInterval(interval);
    } else {
      setIsBlinking(false);
    }
  }, [showSOSModal]);

  return (
    <div className="h-full flex flex-col">
      <div className="p-4 bg-blue-600 text-white">
        <h2 className="text-xl font-bold">Thông tin người dùng</h2>
      </div>

      <div className="flex-1 overflow-y-auto p-4 space-y-4">
        {/* User Info */}
        <div className="bg-white rounded-lg shadow p-4">
          <button
            onClick={() => setExpandedSection(expandedSection === 'info' ? null : 'info')}
            className="w-full flex justify-between items-center font-semibold text-lg mb-2"
          >
            <span>Thông tin cá nhân</span>
            <span>{expandedSection === 'info' ? '▼' : '▶'}</span>
          </button>
          {expandedSection === 'info' && (
            <div className="space-y-2 text-sm">
              <div>
                <span className="font-semibold">Tên:</span> {profile.name}
              </div>
              <div>
                <span className="font-semibold">ID người dùng:</span>{' '}
                <span className="bg-yellow-100 px-2 py-1 rounded font-mono text-xs">
                  {profile.id}
                </span>
              </div>
              <div className="text-xs text-gray-500 mt-2">
                Lưu ý: ID này sẽ được dùng để cặp với thiết bị hộp đen phần cứng
              </div>
              <div>
                <span className="font-semibold">Email:</span> {profile.email}
              </div>
              <div>
                <span className="font-semibold">SĐT:</span> {profile.phone}
              </div>
              <div>
                <span className="font-semibold">Địa chỉ:</span> {profile.address}
              </div>
              <div>
                <span className="font-semibold">Vị trí:</span> {profile.lat?.toFixed(4)}, {profile.lon?.toFixed(4)}
              </div>
            </div>
          )}
        </div>

        {/* SOS Button */}
        <div className="bg-white rounded-lg shadow p-4 relative">
          <button
            onClick={onSendSOS}
            className={`w-full py-3 px-4 rounded-md font-semibold text-lg transition-all ${
              isBlinking 
                ? 'bg-yellow-500 text-black animate-pulse' 
                : 'bg-red-600 text-white hover:bg-red-700'
            }`}
          >
            🚨 Gửi SOS
          </button>
        </div>

        {/* Find Nearest Stations */}
        <div className="bg-white rounded-lg shadow p-4">
          <h3 className="font-semibold mb-3">Tìm trạm gần nhất</h3>
          <div className="space-y-2">
            <button
              onClick={() => onFindNearestStation('medical')}
              className="w-full bg-red-500 text-white py-2 px-4 rounded-md hover:bg-red-600"
            >
              🏥 Tìm trạm y tế gần nhất
            </button>
            <button
              onClick={() => onFindNearestStation('rescue')}
              className="w-full bg-orange-500 text-white py-2 px-4 rounded-md hover:bg-orange-600"
            >
              🔧 Tìm trạm cứu hộ gần nhất
            </button>
          </div>
        </div>

        {/* SOS History */}
        <div className="bg-white rounded-lg shadow p-4">
          <button
            onClick={() => setExpandedSection(expandedSection === 'sos' ? null : 'sos')}
            className="w-full flex justify-between items-center font-semibold mb-2"
          >
            <span>Lịch sử SOS ({sosList.length})</span>
            <span>{expandedSection === 'sos' ? '▼' : '▶'}</span>
          </button>
          {expandedSection === 'sos' && (
            <div className="space-y-2 max-h-64 overflow-y-auto">
              {sosList.length === 0 ? (
                <p className="text-sm text-gray-500">Chưa có lịch sử SOS</p>
              ) : (
                sosList.map((sos) => (
                  <div key={sos.id} className="border-l-4 border-red-500 pl-3 py-2 text-sm">
                    <div className="font-semibold">
                      {sos.type === 'accident' && '🚗 Tai nạn'}
                      {sos.type === 'breakdown' && '🔧 Hỏng xe'}
                      {sos.type === 'medical' && '🏥 Y tế'}
                      {sos.type === 'other' && '⚠️ Khác'}
                    </div>
                    <div className="text-xs text-gray-600">
                      Mức độ: {sos.severity} | Trạng thái: {sos.status}
                    </div>
                    <div className="text-xs text-gray-500">
                      {new Date(sos.createdAt).toLocaleString('vi-VN')}
                    </div>
                    {sos.status === 'done' && sos.assignedStationId && (
                      <button
                        onClick={() => onRateStation({ id: sos.assignedStationId })}
                        className="mt-2 text-xs bg-blue-500 text-white px-2 py-1 rounded hover:bg-blue-600"
                      >
                        Đánh giá trạm
                      </button>
                    )}
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

