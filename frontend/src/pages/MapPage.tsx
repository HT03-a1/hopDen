import { useEffect, useState } from 'react';
import { MapContainer, TileLayer, useMap, Polyline } from 'react-leaflet';
import { Icon } from 'leaflet';
import { useAuthStore } from '../store/authStore';
import apiClient from '../api/client';
import { io } from 'socket.io-client';
import 'leaflet/dist/leaflet.css';
import UserSidePanel from '../components/UserSidePanel';
import StationSidePanel from '../components/StationSidePanel';
import SOSModal from '../components/SOSModal';
import StationDetailModal from '../components/StationDetailModal';
import RatingModal from '../components/RatingModal';
import BlinkingUserMarker from '../components/BlinkingUserMarker';
// Logo cổ loa và CLBSTEM - sử dụng đường dẫn public
const logoColoa = '/logo/logocoloa.png';
const logoCLBSTEM = '/logo/CLBSTEM.jpg';

// Fix default marker icon
delete (Icon.Default.prototype as any)._getIconUrl;
Icon.Default.mergeOptions({
  iconRetinaUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.9.4/images/marker-icon-2x.png',
  iconUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.9.4/images/marker-icon.png',
  shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.9.4/images/marker-shadow.png',
});

interface MapEntity {
  type: string;
  id: string;
  name: string;
  lat: number;
  lon: number;
  [key: string]: any;
}

interface RoutePoint {
  lat: number;
  lon: number;
}

function MapController({ center, zoom }: { center: [number, number]; zoom: number }) {
  const map = useMap();
  useEffect(() => {
    map.setView(center, zoom);
    // Store map instance globally for route zooming
    (window as any).mapInstance = map;
  }, [center, zoom, map]);
  return null;
}

export default function MapPage() {
  const { profile, logout, updateProfile } = useAuthStore();
  const [entities, setEntities] = useState<MapEntity[]>([]);
  const [selectedEntity, setSelectedEntity] = useState<MapEntity | null>(null);
  const [sosList, setSosList] = useState<any[]>([]);
  // Lưu danh sách SOS done/cancelled để kiểm tra ẩn user marker (bảo vệ quyền riêng tư)
  const [doneSOSList, setDoneSOSList] = useState<any[]>([]);
  // Lưu lịch sử SOS đã hoàn thành/hủy để hiển thị cho trạm
  const [historySOSList, setHistorySOSList] = useState<any[]>([]);
  const [showSOSModal, setShowSOSModal] = useState(false);
  const [showStationModal, setShowStationModal] = useState(false);
  const [showRatingModal, setShowRatingModal] = useState(false);
  const [selectedStation, setSelectedStation] = useState<any>(null);
  const [route, setRoute] = useState<RoutePoint[]>([]);
  const [loading, setLoading] = useState(true);

  const [userCurrentLocation, setUserCurrentLocation] = useState<[number, number] | null>(null);

  // Set userCurrentLocation từ profile (ESP32 hoặc nhập thủ công)
  // Cập nhật mỗi khi profile thay đổi để đảm bảo SOSModal luôn có vị trí mới nhất
  useEffect(() => {
    if (profile?.type === 'user' && profile.lat && profile.lon) {
      setUserCurrentLocation([profile.lat, profile.lon]);
      // Reduced logging - removed debug logs
    }
  }, [profile?.lat, profile?.lon, profile?.lastLocationUpdatedAt]);

  // Get map center based on user/station
  const getMapCenter = (): [number, number] => {
    // For user, use current location if available
    if (profile?.type === 'user' && userCurrentLocation) {
      return userCurrentLocation;
    }
    if (profile?.lat && profile?.lon) {
      return [profile.lat, profile.lon];
    }
    return [10.7769, 106.7009]; // Default to Ho Chi Minh City
  };

  // Load map entities
  useEffect(() => {
    loadMapData();
    
    // Tự động reload map data mỗi 5 giây để đảm bảo vị trí luôn được cập nhật
    // (fallback nếu WebSocket không hoạt động)
    const interval = setInterval(() => {
      if (profile?.type === 'user') {
        loadMapData();
      }
    }, 5000);
    
    return () => clearInterval(interval);
  }, [profile]);

  // Setup WebSocket
  useEffect(() => {
    const newSocket = io('http://localhost:3000');

    newSocket.on('sos:new', () => {
      loadMapData();
      loadSOSList();
    });

    newSocket.on('sos:update', async () => {
      // Tự động reload SOS list và map data khi có update
      await loadSOSList();
      loadMapData();
    });

    // Lắng nghe event riêng cho reassign để đảm bảo cập nhật ngay lập tức
    newSocket.on('sos:reassigned', async () => {
      // Tự động reload SOS list và map data khi có reassign
      await loadSOSList();
      loadMapData();
    });

    newSocket.on('user:update', async (data) => {
      const { profile: currentProfile, updateProfile } = useAuthStore.getState();
      
      // Luôn reload map data TRƯỚC để đảm bảo marker được cập nhật ngay lập tức
      await loadMapData();
      
      if (currentProfile?.id === data.id) {
        // Cập nhật đầy đủ thông tin vị trí trong store
        updateProfile({ 
          lat: data.lat, 
          lon: data.lon,
          lastLocationSource: data.lastLocationSource,
          lastLocationUpdatedAt: data.lastLocationUpdatedAt
        });
        setUserCurrentLocation([data.lat, data.lon]);
      }
    });

    return () => {
      newSocket.close();
    };
  }, []);

  const loadMapData = async () => {
    try {
      const response = await apiClient.get('/map/entities');
      setEntities(response.data);
      setLoading(false);
    } catch (error) {
      console.error('❌ Error loading map data:', error);
      setLoading(false);
    }
  };

  const loadSOSList = async () => {
    try {
      if (profile?.type === 'user') {
        const response = await apiClient.get(`/sos?userId=${profile.id}`);
        setSosList(response.data);
      } else if (profile?.type === 'medical' || profile?.type === 'rescue') { // Đã gộp repair vào rescue
        // Load chỉ các SOS đang active (pending, accepted, on_route) - không hiển thị done/cancelled
        const [pendingResponse, assignedResponse] = await Promise.all([
          apiClient.get(`/sos?status=pending`),
          apiClient.get(`/sos?stationId=${profile.id}`)
        ]);
        
        const allowedTypes =
          profile.type === 'medical'
            ? ['medical', 'accident']
            : ['breakdown', 'other'];

        const filterByType = (sos: any) => allowedTypes.includes(sos.type);

        // Gộp danh sách và chỉ lấy các SOS active (pending, accepted, on_route)
        const pendingSOS = (pendingResponse.data || []).filter(filterByType);
        const assignedSOS = (assignedResponse.data || []).filter(
          (sos: any) => sos.status === 'accepted' || sos.status === 'on_route'
        ).filter(filterByType);
        
        // Tính thời gian hiện tại và thời gian 24 giờ trước (chỉ hiển thị SOS trong 24h gần đây)
        const now = new Date();
        const oneDayAgo = new Date(now.getTime() - 24 * 60 * 60 * 1000);
        
        // Tạo Set để loại bỏ trùng lặp theo ID và chỉ giữ các SOS active và mới
        const sosMap = new Map();
        [...pendingSOS, ...assignedSOS].forEach((sos: any) => {
          // Chỉ thêm các SOS active (không phải done hoặc cancelled)
          if (sos.status !== 'done' && sos.status !== 'cancelled') {
            // Kiểm tra thời gian: chỉ hiển thị SOS được tạo trong 24 giờ gần đây
            const sosCreatedAt = new Date(sos.createdAt);
            if (sosCreatedAt >= oneDayAgo) {
              sosMap.set(sos.id, sos);
            }
          }
        });
        
        setSosList(Array.from(sosMap.values()));
        
        // Load thêm các SOS done/cancelled được gán cho trạm này để kiểm tra ẩn user marker
        const allAssignedSOS = (assignedResponse.data || []).filter(filterByType);
        const doneCancelledSOS = allAssignedSOS.filter(
          (sos: any) => (sos.status === 'done' || sos.status === 'cancelled') && sos.assignedStationId === profile.id
        );
        setDoneSOSList(doneCancelledSOS);
        console.log('Done/Cancelled SOS for privacy check:', doneCancelledSOS.length);
        
        // Load lịch sử SOS (done/cancelled) trong 30 ngày gần đây để hiển thị
        const thirtyDaysAgo = new Date(now.getTime() - 30 * 24 * 60 * 60 * 1000);
        const historySOS = doneCancelledSOS.filter((sos: any) => {
          const sosDate = new Date(sos.updatedAt || sos.createdAt);
          return sosDate >= thirtyDaysAgo;
        });
        setHistorySOSList(historySOS);
        console.log('History SOS (last 30 days):', historySOS.length);
      }
    } catch (error) {
      console.error('Error loading SOS list:', error);
    }
  };

  useEffect(() => {
    loadSOSList();
  }, [profile]);

  const getMarkerColor = (type: string) => {
    switch (type) {
      case 'user':
      case 'device':
        return 'black';
      case 'medical_station':
        return 'green';
      case 'rescue_station': // Đã gộp repair_station vào rescue_station
        return 'blue';
      case 'sos':
        return 'red';
      default:
        return 'blue';
    }
  };

  const createCustomIcon = (color: string, isSOS: boolean = false, isLarge: boolean = false) => {
    let iconUrl = 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-blue.png';
    
    if (isSOS) {
      // SOS marker màu đỏ
      iconUrl = 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-red.png';
    } else if (color === 'green') {
      iconUrl = 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-green.png';
    } else if (color === 'red') {
      iconUrl = 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-red.png';
    } else if (color === 'yellow') {
      iconUrl = 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-yellow.png';
    } else if (color === 'orange') {
      iconUrl = 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-orange.png';
    } else if (color === 'blue') {
      iconUrl = 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-blue.png';
    } else if (color === 'black') {
      iconUrl = 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-black.png';
    }
    
    // Make icon larger if isLarge is true (for SOS markers and users with active SOS)
    // Increase size by 1.6x for better visibility (40x65 instead of 25x41)
    const iconSize = isLarge ? [40, 65] : [25, 41];
    const iconAnchor = isLarge ? [20, 65] : [12, 41];
    const popupAnchor = isLarge ? [1, -54] : [1, -34];
    
    return new Icon({
      iconUrl,
      shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.9.4/images/marker-shadow.png',
      iconSize: iconSize as [number, number],
      iconAnchor: iconAnchor as [number, number],
      popupAnchor: popupAnchor as [number, number],
    });
  };

  const handleFindNearestStation = async (type: 'medical' | 'rescue') => {
    // Get location based on account type
    let searchLat: number;
    let searchLon: number;
    
    if (profile?.type === 'user') {
      // For user: use GPS current location, fallback to profile location
      if (userCurrentLocation) {
        searchLat = userCurrentLocation[0];
        searchLon = userCurrentLocation[1];
        console.log('User: Using GPS current location:', searchLat, searchLon);
      } else if (profile?.lat && profile?.lon) {
        // Try to get current location first
        if (navigator.geolocation) {
          try {
            const position = await new Promise<GeolocationPosition>((resolve, reject) => {
              navigator.geolocation.getCurrentPosition(resolve, reject, {
                enableHighAccuracy: true,
                timeout: 5000,
                maximumAge: 0
              });
            });
            searchLat = position.coords.latitude;
            searchLon = position.coords.longitude;
            setUserCurrentLocation([searchLat, searchLon]);
            console.log('User: Got GPS location:', searchLat, searchLon);
          } catch (error) {
            // Fallback to profile location
            searchLat = profile.lat;
            searchLon = profile.lon;
            console.log('User: Using profile location (GPS failed):', searchLat, searchLon);
          }
        } else {
          searchLat = profile.lat;
          searchLon = profile.lon;
          console.log('User: Using profile location (no GPS):', searchLat, searchLon);
        }
      } else {
        alert('Không thể lấy vị trí hiện tại. Vui lòng cho phép truy cập vị trí.');
        return;
      }
    } else if (profile?.type === 'medical' || profile?.type === 'rescue') { // Đã gộp repair vào rescue
      // For station: use station's fixed location
      if (profile?.lat && profile?.lon) {
        searchLat = profile.lat;
        searchLon = profile.lon;
        console.log(`Station (${profile.type}): Using station location:`, searchLat, searchLon);
      } else {
        alert('Trạm không có thông tin vị trí.');
        return;
      }
    } else {
      alert('Không xác định được loại tài khoản.');
      return;
    }

    try {
      const response = await apiClient.get(
        `/stations/nearest?lat=${searchLat}&lon=${searchLon}&type=${type}`
      );
      const station = response.data;
      
      console.log(`Found nearest ${type} station:`, station.stationName, `at distance ${station.distance?.toFixed(2)} km`);

      // Find station in entities
      const stationEntity = entities.find(e => e.id === station.id);
      if (stationEntity) {
        setSelectedEntity(stationEntity);
        setShowStationModal(true);
      }

      // Load route
      try {
        const routeResponse = await apiClient.get(
          `/map/routes?fromLat=${searchLat}&fromLon=${searchLon}&toLat=${station.lat}&toLon=${station.lon}`
        );
        setRoute(routeResponse.data.polyline);
        
        // Auto zoom to fit both points
        const mapInstance = (window as any).mapInstance;
        if (mapInstance && typeof mapInstance.fitBounds === 'function') {
          const bounds = [
            [searchLat, searchLon],
            [station.lat, station.lon]
          ] as [[number, number], [number, number]];
          mapInstance.fitBounds(bounds, { padding: [50, 50] });
        }
      } catch (error) {
        console.error('Error loading route:', error);
      }
    } catch (error) {
      console.error('Error finding nearest station:', error);
      alert('Không tìm thấy trạm gần nhất');
    }
  };

  const handleShowRoute = async (fromLat: number, fromLon: number, toLat: number, toLon: number) => {
    console.log('handleShowRoute called with:', { fromLat, fromLon, toLat, toLon });
    
    // Validate coordinates
    if (isNaN(fromLat) || isNaN(fromLon) || isNaN(toLat) || isNaN(toLon)) {
      console.error('Invalid coordinates in handleShowRoute:', { fromLat, fromLon, toLat, toLon });
      alert('Tọa độ không hợp lệ. Vui lòng thử lại.');
      return;
    }
    
    try {
      const response = await apiClient.get(
        `/map/routes?fromLat=${fromLat}&fromLon=${fromLon}&toLat=${toLat}&toLon=${toLon}`
      );
      console.log('Route response received:', response.data);
      setRoute(response.data.polyline);
      
      // Auto zoom to fit both points
      const mapInstance = (window as any).mapInstance;
      if (mapInstance && typeof mapInstance.fitBounds === 'function') {
        const bounds = [
          [fromLat, fromLon],
          [toLat, toLon]
        ] as [[number, number], [number, number]];
        mapInstance.fitBounds(bounds, { padding: [50, 50] });
      }
    } catch (error) {
      console.error('Error loading route:', error);
      alert('Không thể tải đường đi. Vui lòng thử lại.');
    }
  };

  const handleSOSCreated = () => {
    loadMapData();
    loadSOSList();
  };

  if (loading) {
    return (
      <div className="w-full h-screen flex items-center justify-center">
        <div className="text-xl">Đang tải...</div>
      </div>
    );
  }

  return (
    <div className="w-full h-screen flex flex-col">
      {/* Top Bar - Enhanced Design */}
      <div className="bg-gradient-to-r from-slate-700 via-slate-800 to-slate-900 text-white px-6 py-4 flex justify-between items-center shadow-lg border-b-2 border-slate-950">
        <div className="flex items-center space-x-6">
          {/* Logo/Icon Section */}
          <div className="flex items-center space-x-3">
            <div className="flex items-center space-x-2">
              <div className="bg-white/20 backdrop-blur-sm p-2 rounded-lg flex items-center justify-center">
                <img 
                  src={logoColoa} 
                  alt="Logo cổ loa" 
                  className="w-8 h-8 object-contain"
                />
              </div>
              <div className="bg-white/20 backdrop-blur-sm p-2 rounded-lg flex items-center justify-center">
                <img 
                  src={logoCLBSTEM} 
                  alt="Logo CLBSTEM" 
                  className="w-8 h-8 object-contain"
                />
              </div>
            </div>
            <h1 className="text-2xl font-bold tracking-tight">Hộp đen thông minh</h1>
          </div>
          
          {/* User/Station Info */}
          {profile && (
            <div className="flex items-center space-x-3 pl-4 border-l border-white/30">
              {profile.type === 'user' ? (
                <>
                  <div className="flex items-center space-x-2 bg-white/10 backdrop-blur-sm px-3 py-1.5 rounded-full">
                    <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M16 7a4 4 0 11-8 0 4 4 0 018 0zM12 14a7 7 0 00-7 7h14a7 7 0 00-7-7z" />
                    </svg>
                    <span className="font-semibold text-sm">{profile.name}</span>
                    <span className="text-slate-200 text-xs font-mono bg-white/10 px-2 py-0.5 rounded">ID: {profile.id}</span>
                  </div>
                </>
              ) : (
                <div className="flex items-center space-x-2 bg-white/10 backdrop-blur-sm px-3 py-1.5 rounded-full">
                  {profile.type === 'medical' ? (
                    <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 21V5a2 2 0 00-2-2H7a2 2 0 00-2 2v16m14 0h2m-2 0h-5m-9 0H3m2 0h5M9 7h1m-1 4h1m4-4h1m-1 4h1m-5 10v-5a1 1 0 011-1h2a1 1 0 011 1v5m-4 0h4" />
                    </svg>
                  ) : (
                    <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z" />
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" />
                    </svg>
                  )}
                  <span className="font-semibold text-sm">{profile.stationName}</span>
                  <span className="text-slate-200 text-xs">{profile.type === 'medical' ? '🏥 Trạm y tế' : '🚑 Trạm cứu hộ'}</span>
                </div>
              )}
            </div>
          )}
        </div>
        
        {/* Logout Button */}
        <button
          onClick={logout}
          className="flex items-center space-x-2 bg-red-500 hover:bg-red-600 active:bg-red-700 px-4 py-2 rounded-lg transition-all duration-200 shadow-md hover:shadow-lg transform hover:scale-105 active:scale-95"
        >
          <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M17 16l4-4m0 0l-4-4m4 4H7m6 4v1a3 3 0 01-3 3H6a3 3 0 01-3-3V7a3 3 0 013-3h4a3 3 0 013 3v1" />
          </svg>
          <span className="font-medium">Đăng xuất</span>
        </button>
      </div>

      <div className="flex-1 flex overflow-hidden">
        {/* Map */}
        <div className="flex-1 relative">
          <MapContainer
            center={getMapCenter()}
            zoom={13}
            style={{ height: '100%', width: '100%' }}
          >
            <MapController center={getMapCenter()} zoom={13} key={userCurrentLocation ? `${userCurrentLocation[0]}-${userCurrentLocation[1]}` : 'default'} />
            <TileLayer
              attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
              url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
            />

                       {/* Render user's current location marker if available */}
                       {profile?.type === 'user' && userCurrentLocation && (() => {
              const hasActiveSOS = sosList.some(sos => sos.status !== 'done' && sos.status !== 'cancelled');
              // Nếu user có SOS active, không hiển thị user marker (vì đã có SOS marker)
              if (hasActiveSOS) {
                return null;
              }
              return (
                <BlinkingUserMarker
                  key={`user-current-${profile.id}`}
                  entity={{
                    type: 'user',
                    id: profile.id,
                    name: profile.name,
                    lat: userCurrentLocation[0],
                    lon: userCurrentLocation[1],
                  }}
                  isCurrentUser={true}
                  color={getMarkerColor('user')}
                  isSOS={false}
                  isLarge={false}
                  createCustomIcon={createCustomIcon}
                  shouldBlink={showSOSModal}
                  hasActiveSOS={false}
                  onMarkerClick={(entity) => {
                    setSelectedEntity(entity);
                  }}
                />
              );
            })()}

            {/* Render entities */}
            {entities.map((entity) => {
              // Skip user entity if we're showing current location
              if (profile?.type === 'user' && entity.type === 'user' && entity.id === profile.id && userCurrentLocation) {
                return null;
              }
              
              // Check if this is the current user's marker
              const isCurrentUser = profile?.type === 'user' && entity.type === 'user' && entity.id === profile.id;
              // Check if user has active SOS - nếu có thì ẩn user marker (vì đã có SOS marker)
              const hasActiveSOS = isCurrentUser && sosList.some(sos => sos.status !== 'done' && sos.status !== 'cancelled');
              if (hasActiveSOS && entity.type === 'user') {
                return null; // Ẩn user marker khi có SOS active
              }
              
              // Bảo vệ quyền riêng tư: Ẩn user marker cho trạm nếu user đó có SOS đã done
              if ((profile?.type === 'medical' || profile?.type === 'rescue') && entity.type === 'user') { // Đã gộp repair vào rescue
                // Kiểm tra xem user này có SOS đã done/cancelled được gán cho trạm này không
                const userHasDoneSOS = doneSOSList.some(sos => 
                  sos.userId === entity.id && 
                  sos.assignedStationId === profile.id
                );
                if (userHasDoneSOS) {
                  console.log(`Hiding user marker for privacy: user ${entity.id} has done SOS assigned to station ${profile.id}`);
                  return null; // Ẩn user marker để bảo vệ quyền riêng tư
                }
              }
              
              const color = getMarkerColor(entity.type);
              const isSOS = entity.type === 'sos';
              // SOS markers should be large and always blink
              const isLarge = isSOS;
              // SOS markers luôn nhấp nháy đỏ cho tất cả tài khoản
              const shouldBlink = isSOS;

              return (
                <BlinkingUserMarker
                key={`${entity.type}-${entity.id}`}
                entity={entity}
                isCurrentUser={isCurrentUser}
                color={color}
                isSOS={isSOS}
                isLarge={isLarge}
                createCustomIcon={createCustomIcon}
                shouldBlink={shouldBlink}
                hasActiveSOS={false}
                onMarkerClick={(entity) => {
                    setSelectedEntity(entity);
                    if (entity.type === 'medical_station' || entity.type === 'rescue_station') { // Đã gộp repair_station vào rescue_station
                      setShowStationModal(true);
                    }
                  }}
                />
              );
            })}

            {/* Route polyline */}
            {route.length > 0 && (
              <Polyline
                positions={route.map(p => [p.lat, p.lon] as [number, number])}
                color="blue"
                weight={4}
                opacity={0.7}
              />
            )}
          </MapContainer>

          {/* Legend */}
          <div className="absolute bottom-4 right-4 bg-white p-4 rounded-lg shadow-lg z-[1000]">
            <h3 className="font-bold mb-2 text-sm">Chú giải</h3>
            <div className="space-y-1 text-xs">
              <div className="flex items-center space-x-2">
                <div className="w-4 h-4 bg-black rounded"></div>
                <span>Người dùng / Thiết bị</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className="w-4 h-4 bg-green-500 rounded"></div>
                <span>Trạm y tế</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className="w-4 h-4 bg-blue-500 rounded"></div>
                <span>Trạm cứu hộ / Sửa xe</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className="w-4 h-4 bg-red-500 rounded animate-pulse"></div>
                <span>SOS (nhấp nháy đỏ, to)</span>
              </div>
            </div>
          </div>
        </div>

        {/* Side Panel */}
        <div className="w-96 bg-gray-50 border-l border-gray-200 overflow-y-auto relative">
          {profile?.type === 'user' ? (
            <>
              <UserSidePanel
                profile={profile}
                sosList={sosList}
                onSendSOS={() => setShowSOSModal(true)}
                onFindNearestStation={handleFindNearestStation}
                onShowRoute={handleShowRoute}
                onRateStation={(station) => {
                  setSelectedStation(station);
                  setShowRatingModal(true);
                }}
                showSOSModal={showSOSModal}
                onCancelSOS={async (sosId) => {
                  try {
                    await apiClient.patch(`/sos/${sosId}/status`, { status: 'cancelled' });
                    loadSOSList();
                    loadMapData();
                  } catch (error) {
                    console.error('Error cancelling SOS:', error);
                    alert('Không thể hủy SOS. Vui lòng thử lại.');
                  }
                }}
                onUpdateLocation={async (lat, lon) => {
                  // Cập nhật profile trong store
                  updateProfile({ lat, lon });
                  // Cập nhật userCurrentLocation và reload map data
                  setUserCurrentLocation([lat, lon]);
                  loadMapData();
                }}
              />
              {/* SOS Modal positioned below SOS button */}
              {showSOSModal && profile && (
                <div className="absolute right-0 top-64 z-50 w-full px-4">
                  <SOSModal
                    userId={profile.id}
                    userLocation={userCurrentLocation ? { lat: userCurrentLocation[0], lon: userCurrentLocation[1] } : undefined}
                    onClose={() => setShowSOSModal(false)}
                    onSuccess={handleSOSCreated}
                  />
                </div>
              )}
            </>
          ) : (
            <StationSidePanel
              profile={profile}
              sosList={sosList}
              historySOSList={historySOSList}
              onShowRoute={handleShowRoute}
              onToggleReady={async (sosId, ready) => {
                try {
                  if (!profile?.id) {
                    alert('Lỗi: Không tìm thấy thông tin trạm');
                    return;
                  }

                  console.log('Toggling ready status:', { sosId, stationId: profile.id, ready });
                  
                  // Toggle ready status
                  await apiClient.patch(`/sos/${sosId}/ready`, {
                    stationId: profile.id,
                    ready: ready,
                  });
                  
                  // Reload data
                  await loadSOSList();
                  await loadMapData();
                } catch (error: any) {
                  console.error('Error toggling ready status:', error);
                  const errorMessage = error.response?.data?.error || error.message || 'Không thể cập nhật trạng thái';
                  alert(`Không thể cập nhật trạng thái: ${errorMessage}`);
                }
              }}
              onClaimSOS={async (sosId) => {
                try {
                  if (!profile?.id) {
                    alert('Lỗi: Không tìm thấy thông tin trạm');
                    return;
                  }

                  console.log('Claiming SOS:', { sosId, stationId: profile.id });
                  
                  // Claim SOS
                  const response = await apiClient.patch(`/sos/${sosId}/claim`, {
                    stationId: profile.id,
                  });
                  
                  console.log('Claim SOS response:', response.data);
                  
                  // Reload data
                  await loadSOSList();
                  await loadMapData();
                  
                  // Get SOS details và tự động hiển thị route trên bản đồ
                  try {
                    const sosResponse = await apiClient.get(`/sos/${sosId}`);
                    const sos = sosResponse.data;
                    
                    // Tự động hiển thị route trên bản đồ
                    if (profile?.lat && profile?.lon && sos.location) {
                      const fromLat = profile.lat;
                      const fromLon = profile.lon;
                      const toLat = sos.location.lat;
                      const toLon = sos.location.lon;
                      
                      // Hiển thị route trên bản đồ
                      await handleShowRoute(fromLat, fromLon, toLat, toLon);
                    }
                  } catch (routeError) {
                    console.error('Error loading SOS details for route:', routeError);
                    // Don't show error to user, route opening is optional
                  }
                } catch (error: any) {
                  console.error('Error claiming SOS:', error);
                  const errorMessage = error.response?.data?.error || error.message || 'Không thể nhận nhiệm vụ';
                  alert(`Không thể nhận nhiệm vụ: ${errorMessage}`);
                }
              }}
                onUpdateStatus={async (sosId, status) => {
                  try {
                    // Khi trạm update status, gửi kèm stationId để backend phân biệt
                    if (!profile?.id) {
                      console.error('Profile ID not available');
                      return;
                    }
                    await apiClient.patch(`/sos/${sosId}/status`, { 
                      status,
                      stationId: profile.id // Gửi stationId để backend biết là trạm đang update
                    });
                    // Reload SOS list (bao gồm cả doneSOSList) để cập nhật danh sách ẩn user marker
                    await loadSOSList();
                    loadMapData();
                    
                    // Nếu hoàn thành nhiệm vụ (done) hoặc hủy (cancelled), xóa route trên bản đồ
                    if (status === 'done' || status === 'cancelled') {
                      setRoute([]);
                    }
                  } catch (error) {
                    console.error('Error updating status:', error);
                  }
                }}
            />
          )}
        </div>
      </div>

      {/* Modals */}
      {showStationModal && selectedEntity && (
        <StationDetailModal
          station={selectedEntity}
          onClose={() => {
            setShowStationModal(false);
            setSelectedEntity(null);
          }}
          onContact={(station) => {
            setSelectedStation(station);
            if (profile?.type === 'user') {
              setShowSOSModal(true);
            }
          }}
        />
      )}

      {showRatingModal && selectedStation && profile && (
        <RatingModal
          stationId={selectedStation.id}
          userId={profile.id}
          onClose={() => {
            setShowRatingModal(false);
            setSelectedStation(null);
          }}
          onSuccess={() => {
            loadMapData();
            setShowRatingModal(false);
            setSelectedStation(null);
          }}
        />
      )}
    </div>
  );
}

