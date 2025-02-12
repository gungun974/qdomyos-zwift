import CoreBluetooth

let FitnessMachineServiceUuid = CBUUID(string: "0x1826");
let FitnessMachineFeatureCharacteristicUuid = CBUUID(string: "0x2ACC");
let supported_resistance_level_rangeCharacteristicUuid = CBUUID(string: "0x2AD6");
let FitnessMachineControlPointUuid = CBUUID(string: "0x2AD9");
let indoorbikeUuid = CBUUID(string: "0x2AD2");
let FitnessMachinestatusUuid = CBUUID(string: "0x2ADA");
let TrainingStatusUuid = CBUUID(string: "0x2AD3");

@objc public class virtualbike_zwift: NSObject {
    private var peripheralManager: BLEPeripheralManagerZwift!
    
    @objc public override init() {
      super.init()
      peripheralManager = BLEPeripheralManagerZwift()
    }
    
    @objc public func updateHeartRate(HeartRate: UInt8)
    {
        peripheralManager.heartRate = HeartRate
    }
    
    @objc public func readCurrentSlope() -> Double
    {
        return peripheralManager.CurrentSlope;
    }

    @objc public func readPowerRequested() -> Double
    {
        return peripheralManager.PowerRequested;
    }
    
    @objc public func updateFTMS(normalizeSpeed: UInt16, currentCadence: UInt16, currentResistance: UInt8, currentWatt: UInt16) -> Bool
    {
        peripheralManager.NormalizeSpeed = normalizeSpeed
        peripheralManager.CurrentCadence = currentCadence
        peripheralManager.CurrentResistance = currentResistance
        peripheralManager.CurrentWatt = currentWatt

        return peripheralManager.connected;
    }
    
    @objc public func getLastFTMSMessage() -> Data? {
        peripheralManager.LastFTMSMessageReceivedAndPassed = peripheralManager.LastFTMSMessageReceived
        peripheralManager.LastFTMSMessageReceived?.removeAll()
        return peripheralManager.LastFTMSMessageReceivedAndPassed
    }
}

class BLEPeripheralManagerZwift: NSObject, CBPeripheralManagerDelegate {
  private var peripheralManager: CBPeripheralManager!

  private var heartRateService: CBMutableService!
  private var heartRateCharacteristic: CBMutableCharacteristic!
  public var heartRate:UInt8! = 0

  private var FitnessMachineService: CBMutableService!
  private var FitnessMachineFeatureCharacteristic: CBMutableCharacteristic!
  private var supported_resistance_level_rangeCharacteristic: CBMutableCharacteristic!
  private var FitnessMachineControlPointCharacteristic: CBMutableCharacteristic!
  private var indoorbikeCharacteristic: CBMutableCharacteristic!
  private var FitnessMachinestatusCharacteristic: CBMutableCharacteristic!
  private var TrainingStatusCharacteristic: CBMutableCharacteristic!
    public var CurrentSlope: Double! = 0
    public var PowerRequested: Double! = 0
    public var NormalizeSpeed: UInt16! = 0
    public var CurrentCadence: UInt16! = 0
    public var CurrentResistance: UInt8! = 0
    public var CurrentWatt: UInt16! = 0
    
    public var LastFTMSMessageReceived: Data?
    public var LastFTMSMessageReceivedAndPassed: Data?
    
    public var serviceToggle: Bool = false

  public var connected: Bool = false

  private var notificationTimer: Timer! = nil
  //var delegate: BLEPeripheralManagerDelegate?

  override init() {
    super.init()
    peripheralManager = CBPeripheralManager(delegate: self, queue: nil)
  }
  
  func peripheralManagerDidUpdateState(_ peripheral: CBPeripheralManager) {
    switch peripheral.state {
    case .poweredOn:
      print("Peripheral manager is up and running")
      
      
      self.heartRateService = CBMutableService(type: heartRateServiceUUID, primary: true)
      let characteristicProperties: CBCharacteristicProperties = [.notify, .read, .write]
      let characteristicPermissions: CBAttributePermissions = [.readable]
      self.heartRateCharacteristic = CBMutableCharacteristic(type:          heartRateCharacteristicUUID,
                                                            properties: characteristicProperties,
                                                            value: nil,
                                                            permissions: characteristicPermissions)
      
      heartRateService.characteristics = [heartRateCharacteristic]
      self.peripheralManager.add(heartRateService)

      self.FitnessMachineService = CBMutableService(type: FitnessMachineServiceUuid, primary: true)

      let FitnessMachineFeatureProperties: CBCharacteristicProperties = [.read]
        let FitnessMachineFeaturePermissions: CBAttributePermissions = [.readable]
        self.FitnessMachineFeatureCharacteristic = CBMutableCharacteristic(type: FitnessMachineFeatureCharacteristicUuid,
                                                               properties: FitnessMachineFeatureProperties,
																					                  value: Data (bytes: [0x83, 0x14, 0x00, 0x00, 0x0c, 0xe0, 0x00, 0x00]),
                                                                                 permissions: FitnessMachineFeaturePermissions)
        
      let supported_resistance_level_rangeProperties: CBCharacteristicProperties = [.read]
        let supported_resistance_level_rangePermissions: CBAttributePermissions = [.readable]
        self.supported_resistance_level_rangeCharacteristic = CBMutableCharacteristic(type: supported_resistance_level_rangeCharacteristicUuid,
                                                         properties: supported_resistance_level_rangeProperties,
                                                                         value: Data (bytes: [0x0A, 0x00, 0x96, 0x00, 0x0A, 0x00]),
                                                                         permissions: supported_resistance_level_rangePermissions)

        let FitnessMachineControlPointProperties: CBCharacteristicProperties = [.indicate, .write]
        let FitnessMachineControlPointPermissions: CBAttributePermissions = [.writeable]
        self.FitnessMachineControlPointCharacteristic = CBMutableCharacteristic(type: FitnessMachineControlPointUuid,
                                                   properties: FitnessMachineControlPointProperties,
                                                                 value: nil,
                                                                 permissions: FitnessMachineControlPointPermissions)

      let indoorbikeProperties: CBCharacteristicProperties = [.notify, .read]
        let indoorbikePermissions: CBAttributePermissions = [.readable]
        self.indoorbikeCharacteristic = CBMutableCharacteristic(type: indoorbikeUuid,
                                             properties: indoorbikeProperties,
                                                         value: nil,
                                                         permissions: indoorbikePermissions)
        
        let FitnessMachinestatusProperties: CBCharacteristicProperties = [.notify]
        let FitnessMachinestatusPermissions: CBAttributePermissions = [.readable]
        self.FitnessMachinestatusCharacteristic = CBMutableCharacteristic(type: FitnessMachinestatusUuid,
                                           properties: FitnessMachinestatusProperties,
                                                       value: nil,
                                                       permissions: FitnessMachinestatusPermissions)
        
        let TrainingStatusProperties: CBCharacteristicProperties = [.read]
        let TrainingStatusPermissions: CBAttributePermissions = [.readable]
        self.TrainingStatusCharacteristic = CBMutableCharacteristic(type: TrainingStatusUuid,
                                                       properties: TrainingStatusProperties,
                                                                       value: Data (bytes: [0x00, 0x01]),
                                                                       permissions: TrainingStatusPermissions)

      FitnessMachineService.characteristics = [FitnessMachineFeatureCharacteristic,
                                               supported_resistance_level_rangeCharacteristic,
                                               FitnessMachineControlPointCharacteristic,
                                               indoorbikeCharacteristic,
                                               FitnessMachinestatusCharacteristic,
                                               TrainingStatusCharacteristic ]
        
        self.peripheralManager.add(FitnessMachineService)

    default:
      print("Peripheral manager is down")
    }
  }
    
  func peripheralManager(_ peripheral: CBPeripheralManager, didAdd service: CBService, error: Error?) {
    if let uwError = error {
      print("Failed to add service with error: \(uwError.localizedDescription)")
      return
    }
    
    let advertisementData = [CBAdvertisementDataLocalNameKey: "QZ",
                              CBAdvertisementDataServiceUUIDsKey: [heartRateServiceUUID, FitnessMachineServiceUuid]] as [String : Any]
    peripheralManager.startAdvertising(advertisementData)
    print("Successfully added service")
  }
  
  
  func peripheralManagerDidStartAdvertising(_ peripheral: CBPeripheralManager, error: Error?) {
    if let uwError = error {
      print("Failed to advertise with error: \(uwError.localizedDescription)")
      return
    }
    
    print("Started advertising")
    
  }
  
    func peripheralManager(_ peripheral: CBPeripheralManager, didReceiveWrite requests: [CBATTRequest]) {
        if requests.first!.characteristic == self.FitnessMachineControlPointCharacteristic {
            if(LastFTMSMessageReceived == nil || LastFTMSMessageReceived?.count == 0) {
                LastFTMSMessageReceived = requests.first!.value
            }
            if(requests.first!.value?.first == 0x11)
          {
			       var high : Int16 = ((Int16)(requests.first!.value![4])) << 8;
					 self.CurrentSlope = (Double)((Int16)(requests.first!.value![3]) + high);
          }
            else if(requests.first!.value?.first == 0x05)
          {
                var high : UInt16 = (((UInt16)(requests.first!.value![2])) << 8);
                self.PowerRequested = (Double)((UInt16)(requests.first!.value![1]) + high);
          }
	  self.connected = true;
          self.peripheralManager.respond(to: requests.first!, withResult: .success)
          print("Responded successfully to a read request")
            
          let funcCode: UInt8 = requests.first!.value![0]
          var response: [UInt8] = [0x80, funcCode , 0x01]
          let responseData = Data(bytes: &response, count: 3)
              
          self.peripheralManager.updateValue(responseData, for: self.FitnessMachineControlPointCharacteristic, onSubscribedCentrals: nil)
      }
    }
    
  func peripheralManager(_ peripheral: CBPeripheralManager, didReceiveRead request: CBATTRequest) {
    if request.characteristic == self.heartRateCharacteristic {
      request.value = self.calculateHeartRate()
      self.peripheralManager.respond(to: request, withResult: .success)
      print("Responded successfully to a read request")
    }
  }
  
  func peripheralManager(_ peripheral: CBPeripheralManager, central: CBCentral, didSubscribeTo characteristic: CBCharacteristic) {
    print("Successfully subscribed")
	 self.connected = true
    updateSubscribers();
    self.startSendingDataToSubscribers()
  }
  
  func peripheralManager(_ peripheral: CBPeripheralManager, central: CBCentral, didUnsubscribeFrom characteristic: CBCharacteristic) {
    //self.notificationTimer.invalidate()
	 self.connected = false
    print("Successfully unsubscribed")
  }

  func startSendingDataToSubscribers() {
    if self.notificationTimer == nil {
        self.notificationTimer = Timer.scheduledTimer(timeInterval: 1, target: self, selector: #selector(self.updateSubscribers), userInfo: nil, repeats: true)
        }
  }

  func peripheralManagerIsReady(toUpdateSubscribers peripheral: CBPeripheralManager) {
    print("Peripheral manager is ready to update subscribers")
    updateSubscribers();
    self.startSendingDataToSubscribers()
  }

  func calculateHeartRate() -> Data {
    //self.delegate?.BLEPeripheralManagerDidSendValue(self.heartRate)
    var heartRateBPM: [UInt8] = [0, self.heartRate, 0, 0, 0, 0, 0, 0]
    let heartRateData = Data(bytes: &heartRateBPM, count: MemoryLayout.size(ofValue: heartRateBPM))
    return heartRateData
  }
    
    func calculateIndoorBike() -> Data {
        let flags0:UInt8 = 0x64
        let flags1:UInt8 = 0x02
      //self.delegate?.BLEPeripheralManagerCSCDidSendValue(flags, crankRevolutions: self.crankRevolutions, lastCrankEventTime: self.lastCrankEventTime)
        var indoorBike: [UInt8] = [flags0, flags1, (UInt8)(self.NormalizeSpeed & 0xFF), (UInt8)((self.NormalizeSpeed >> 8) & 0xFF),  (UInt8)(self.CurrentCadence & 0xFF), (UInt8)((self.CurrentCadence >> 8) & 0xFF), self.CurrentResistance, 0x00, (UInt8)(self.CurrentWatt & 0xFF), (UInt8)((self.CurrentWatt >> 8) & 0xFF),
                                   self.heartRate, 0x00]
      let indoorBikeData = Data(bytes: &indoorBike, count: 12)
      return indoorBikeData
    }
  
  @objc func updateSubscribers() {
    let heartRateData = self.calculateHeartRate()
    let indoorBikeData = self.calculateIndoorBike()
    
    if(self.serviceToggle == true)
    {
        let ok = self.peripheralManager.updateValue(heartRateData, for: self.heartRateCharacteristic, onSubscribedCentrals: nil)
        if(ok) {
            self.serviceToggle = !self.serviceToggle
        }
    }
    else
    {
        let ok = self.peripheralManager.updateValue(indoorBikeData, for: self.indoorbikeCharacteristic, onSubscribedCentrals: nil)
        if(ok) {
            self.serviceToggle = !self.serviceToggle
        }
    }
  }
  
} /// class-end
