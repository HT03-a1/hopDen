import apiClient from '../api/client';
import { useAuthStore } from '../store/authStore';

interface StationDetailModalProps {
  station: any;
  onClose: () => void;
  onContact: (station: any) => void;
}

export default function StationDetailModal({ station, onClose, onContact }: StationDetailModalProps) {
  const { profile } = useAuthStore();

  const handleContact = () => {
    onContact(station);
    onClose();
  };

  return (
    <div 
      className="fixed inset-0 bg-black bg-opacity-50 flex items-start justify-center z-[9999] pt-8"
      onClick={onClose}
      style={{ zIndex: 9999 }}
    >
      <div 
        className="bg-white rounded-lg p-6 w-full max-w-md shadow-2xl max-h-[85vh] overflow-y-auto mx-4"
        onClick={(e) => e.stopPropagation()}
        style={{ zIndex: 10000 }}
      >
        <h2 className="text-2xl font-bold mb-4">{station.name || station.stationName || 'Trạm'}</h2>

        <div className="space-y-3 mb-4">
          <div>
            <span className="font-semibold">ID trạm:</span>{' '}
            <span className="bg-yellow-100 px-2 py-1 rounded font-mono text-xs">{station.id}</span>
          </div>
          <div>
            <span className="font-semibold">Loại trạm:</span>{' '}
            {(station.type === 'medical_station' || station.stationType === 'medical') && '🏥 Trạm y tế'}
            {(station.type === 'rescue_station' || station.stationType === 'rescue') && '🔧 Trạm cứu hộ'}
            {(station.type === 'repair_station' || station.stationType === 'repair') && '🔨 Trạm sửa xe'}
            {!station.type && !station.stationType && <span className="text-gray-500">Chưa xác định</span>}
          </div>
          <div>
            <span className="font-semibold">Tên trạm:</span> {station.name || station.stationName}
          </div>
          <div>
            <span className="font-semibold">Địa chỉ:</span> {station.address || 'Chưa cập nhật'}
          </div>
          <div>
            <span className="font-semibold">Số điện thoại:</span>{' '}
            {station.phone ? (
              <a href={`tel:${station.phone}`} className="text-blue-600 hover:underline">
                {station.phone}
              </a>
            ) : (
              <span className="text-gray-500">Chưa cập nhật</span>
            )}
          </div>
          <div>
            <span className="font-semibold">Email:</span>{' '}
            {station.email ? (
              <a href={`mailto:${station.email}`} className="text-blue-600 hover:underline">
                {station.email}
              </a>
            ) : (
              <span className="text-gray-500">Chưa cập nhật</span>
            )}
          </div>
          {station.openHours && (
            <div>
              <span className="font-semibold">Giờ làm việc:</span> {station.openHours}
            </div>
          )}
          {station.ratingAvg !== undefined && station.ratingAvg > 0 && (
            <div>
              <span className="font-semibold">Đánh giá:</span> ⭐ {station.ratingAvg.toFixed(1)} ({station.ratingCount || 0} lượt)
            </div>
          )}
          {station.description && (
            <div>
              <span className="font-semibold">Mô tả:</span> {station.description}
            </div>
          )}
          <div className="text-sm text-gray-500">
            <span className="font-semibold">Vị trí:</span> {station.lat?.toFixed(4)}, {station.lon?.toFixed(4)}
          </div>
        </div>

        <div className="flex space-x-3">
          <button
            onClick={onClose}
            className="flex-1 bg-gray-300 text-gray-700 py-2 px-4 rounded-md hover:bg-gray-400"
          >
            Đóng
          </button>
          {profile?.type === 'user' && (
            <button
              onClick={handleContact}
              className="flex-1 bg-blue-600 text-white py-2 px-4 rounded-md hover:bg-blue-700"
            >
              Liên hệ trạm này
            </button>
          )}
        </div>
      </div>
    </div>
  );
}

