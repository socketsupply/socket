// vim: set sw=2:
package __BUNDLE_IDENTIFIER__

import java.lang.ref.WeakReference
import java.util.concurrent.Executor
import java.util.concurrent.Executors
import java.util.concurrent.Semaphore

typealias BluetoothGattServer = android.bluetooth.BluetoothGattServer
typealias BluetoothGatt = android.bluetooth.BluetoothGatt
typealias BluetoothDevice = android.bluetooth.BluetoothDevice
typealias UUID = java.util.UUID

interface IBluetoothConfiguration {
  val manager: android.bluetooth.BluetoothManager
  val send: (String, String, String, String, ByteArray?) -> Unit
  val emit: (String, String?) -> Unit
  val pin: String?
}

const val BLUETOOTH_CHARACTERISTIC_PROPERTIES = (
  android.bluetooth.BluetoothGattCharacteristic.PROPERTY_BROADCAST or
  android.bluetooth.BluetoothGattCharacteristic.PROPERTY_INDICATE or
  android.bluetooth.BluetoothGattCharacteristic.PROPERTY_NOTIFY or
  android.bluetooth.BluetoothGattCharacteristic.PROPERTY_WRITE or
  android.bluetooth.BluetoothGattCharacteristic.PROPERTY_READ
)

const val BLUETOOTH_CHARACTERISTIC_PERMISSIONS = (
  android.bluetooth.BluetoothGattCharacteristic.PERMISSION_READ or
  android.bluetooth.BluetoothGattCharacteristic.PERMISSION_WRITE
)

open class BluetoothCharacteristic (
  uuid: UUID
) : android.bluetooth.BluetoothGattCharacteristic(
  uuid,
  BLUETOOTH_CHARACTERISTIC_PROPERTIES,
  BLUETOOTH_CHARACTERISTIC_PERMISSIONS
)

open class BluetoothService (
  uuid: UUID
) : android.bluetooth.BluetoothGattService(
  uuid,
  android.bluetooth.BluetoothGattService.SERVICE_TYPE_PRIMARY
)

data class BluetoothConfiguration (
  override val manager: android.bluetooth.BluetoothManager,
  override val send: (String, String, String, String, ByteArray?) -> Unit,
  override val emit: (String, String?) -> Unit,
  override val pin: String? = Bluetooth.PIN_DEFAULT_VALUE
) : IBluetoothConfiguration

typealias BluetoothCommandFunction = (BluetoothGatt) -> Int
open class BluetoothCommand (
  queue: BluetoothCommandQueue,
  bluetooth: Bluetooth,
  function: BluetoothCommandFunction
) {
  companion object {
    val STATUS_CONTINUE = -2
    val STATUS_PENDING = -1
    val STATUS_SUCCESS = 0
    val STATUS_FAILURE = 1
  }

  val bluetooth = bluetooth
  val function = function
  var status = STATUS_PENDING
  val queue = queue
  val ts = System.currentTimeMillis() / 1000

  open fun run (gatt: BluetoothGatt): Int {
    if (status == STATUS_PENDING) {
      status = function(gatt)
    }

    return status
  }

  open operator fun invoke (gatt: BluetoothGatt): Int {
    return run(gatt)
  }
}

open class BluetoothCommandQueue (
  bluetooth: Bluetooth,
  gatt: BluetoothGatt
) {
  val bluetooth = bluetooth
  val semaphore = Semaphore(1, true)
  val commands = mutableListOf<BluetoothCommand>()
  val executor = Executors.newFixedThreadPool(1)
  val gatt = WeakReference(gatt)

  open fun push (function: BluetoothCommandFunction) {
    commands.add(BluetoothCommand(this, bluetooth, function))
  }

  open fun shift (): BluetoothCommand? {
    if (commands.size == 0) return null
    val index = 0
    val command = commands.elementAt(index)
    commands.removeAt(index)
    return command
  }

  open fun peek (): BluetoothCommand? {
    if (commands.size == 0) return null
    val index = 0
    val command = commands.elementAt(index)
    return command
  }

  open fun dequeue (): Boolean {
    synchronized(commands) {
      val semaphore = this.semaphore
      val command = peek() ?: return false
      val gatt = this.gatt.get() ?: return false

      executor.execute({
        console.log("before lock")
        semaphore.acquireUninterruptibly()
        console.log("after lock")
        val status = command(gatt)
        console.log("after command")

        if (status == BluetoothCommand.STATUS_CONTINUE) {
          console.log("continue")
          this.bluetooth.handler.postDelayed({ this.next() }, 0)
        }

        console.log("after exec")
      })

      return true
    }
  }

  open fun next (): Boolean {
    semaphore.release()
    console.log("release")

    if (this.dequeue()) {
      synchronized(commands) {
        val command = shift()
        return command != null
      }
    }

    return false
  }
}

open class BluetoothScanCallback (
  bluetooth: Bluetooth
) : android.bluetooth.le.ScanCallback() {
  val bluetooth = bluetooth

  override fun onBatchScanResults (results: List<android.bluetooth.le.ScanResult>) {
    super.onBatchScanResults(results)
    for (result in results) {
      this.handleResult(result)
    }
  }

  override fun onScanFailed (errorCode: Int) {
    super.onScanFailed(errorCode)
  }

  override fun onScanResult (
    callbackType: Int,
    result: android.bluetooth.le.ScanResult
  ) {
    super.onScanResult(callbackType, result)
    this.handleResult(result)
  }

  fun handleResult (result: android.bluetooth.le.ScanResult) {
    val isConnectable = result.isConnectable()
    val device = result.device

    this.bluetooth.onDevice(device, isConnectable)

    val gatt = this.bluetooth.gatts[device.address]
    val record = result.scanRecord
    val uuids = record?.serviceUuids

    if (gatt != null && uuids != null) {
      for (uuid in uuids) {
        val service = gatt.getService(uuid.uuid)
        console.log("result device had uuid= ${uuid.toString()} connectable=${isConnectable}")
        if (service != null) {
          console.log("result had service for uuid= ${uuid.toString()}")
        }
      }
    }
  }
}

open class BluetoothAdvertiseCallback (
  bluetooth: Bluetooth
) : android.bluetooth.le.AdvertiseCallback() {
  val bluetooth = bluetooth
}

open class BluetoothGattServerClientCallback (
  bluetooth: Bluetooth
) : android.bluetooth.BluetoothGattCallback() {
  val bluetooth = bluetooth

  override fun onCharacteristicRead (
    gatt: android.bluetooth.BluetoothGatt,
    characteristic: android.bluetooth.BluetoothGattCharacteristic,
    value: ByteArray,
    status: Int
  ) {
    console.log("onCharacteristicRead($status): ${String(value)}")
    super.onCharacteristicRead(gatt, characteristic, value, status)
    this.bluetooth.getDeviceQueue(gatt.device.address)?.next()
  }

  override fun onCharacteristicWrite (
    gatt: android.bluetooth.BluetoothGatt,
    characteristic: android.bluetooth.BluetoothGattCharacteristic,
    status: Int
  ) {
    console.log("onCharacteristicWrite($status)")
    super.onCharacteristicWrite(gatt, characteristic, status)
    this.bluetooth.getDeviceQueue(gatt.device.address)?.next()
  }

  override fun onCharacteristicChanged (
    gatt: android.bluetooth.BluetoothGatt,
    characteristic: android.bluetooth.BluetoothGattCharacteristic,
    value: ByteArray
  ) {
    console.log("onCharacteristicChanged: ${value.toString()}")
    super.onCharacteristicChanged(gatt, characteristic, value)
    this.bluetooth.getDeviceQueue(gatt.device.address)?.next()
  }

  override fun onConnectionStateChange (
    gatt: android.bluetooth.BluetoothGatt,
    status: Int,
    state: Int
  ) {
    console.log("onConnectionStateChange: $status $state")
    super.onConnectionStateChange(gatt, status, state)

    val device = gatt.device
    val uuids = device.uuids ?: emptyArray()
    val uuid = try { uuids[0]?.toString() } catch (_: Exception) { "" }

    val address = device.address ?: null
    val name = device.name ?: device.alias ?: device.address ?: null

    if (state == android.bluetooth.BluetoothProfile.STATE_CONNECTED) {
      val queue = this.bluetooth.addToDeviceQueue(gatt.device.address, { g ->
        if (g.discoverServices()) {
          BluetoothCommand.STATUS_SUCCESS
        } else {
          BluetoothCommand.STATUS_FAILURE
        }
      })

      queue?.next()

      this.bluetooth.emit("""{
        "event": "connect",
        "address": ${if (address != null) "\"$address\"" else "null"},
        "name": ${if (name != null) "\"$name\"" else "null"},
        "uuid": "$uuid"
      }""", null)

    } else if (status != 133) {
      this.bluetooth.emit("""{
        "event": "disconnect",
        "address": ${if (address != null) "\"$address\"" else "null"},
        "name": ${if (name != null) "\"$name\"" else "null"},
        "uuid": "$uuid"
      }""", null)

      val queue = this.bluetooth.addToDeviceQueue(gatt.device.address, { g ->
        this.bluetooth.onDevice(gatt.device, true, g)
        if (g.connect()) {
          BluetoothCommand.STATUS_SUCCESS
        } else {
          BluetoothCommand.STATUS_FAILURE
        }
      })

      queue?.next()
    }
  }

  override fun onDescriptorRead (
    gatt: android.bluetooth.BluetoothGatt,
    descriptor: android.bluetooth.BluetoothGattDescriptor,
    status: Int,
    value: ByteArray
  ) {
    console.log("onDescriptorRead($status)")
    super.onDescriptorRead(gatt, descriptor, status, value)
    this.bluetooth.getDeviceQueue(gatt.device.address)?.next()
  }

  override fun onDescriptorWrite (
    gatt: android.bluetooth.BluetoothGatt,
    descriptor: android.bluetooth.BluetoothGattDescriptor,
    status: Int
  ) {
    console.log("onDescriptorWrite($status)")
    super.onDescriptorWrite(gatt, descriptor, status)
    this.bluetooth.getDeviceQueue(gatt.device.address)?.next()
  }

  override fun onReliableWriteCompleted (
    gatt: android.bluetooth.BluetoothGatt,
    status: Int
  ) {
    super.onReliableWriteCompleted(gatt, status)
  }

  override fun onServiceChanged (
    gatt: android.bluetooth.BluetoothGatt
  ) {
    console.log("onServiceChanged")
    super.onServiceChanged(gatt)
    this.bluetooth.stopAdvertising()
    this.bluetooth.startAdvertising()
  }

  override fun onServicesDiscovered (
    gatt: android.bluetooth.BluetoothGatt,
    status: Int
  ) {
    console.log("onServicesDiscovered")
    super.onServicesDiscovered(gatt, status)
    val queue = this.bluetooth.getDeviceQueue(gatt.device.address)

    for (service in gatt.services) {
      if (this.bluetooth.server?.getService(service.uuid) != null) {
        console.log("service: ${service.uuid.toString()}")
        for (characteristic in service.characteristics) {
          console.log("characteristic: ${characteristic.uuid.toString()}")
          queue?.push({ g ->
            if (g.readCharacteristic(characteristic)) {
              BluetoothCommand.STATUS_SUCCESS
            } else {
              BluetoothCommand.STATUS_FAILURE
            }
          })
        }
      }
    }

    queue?.next()
  }
}

open class BluetoothGattServerServerCallback (
  bluetooth: Bluetooth
) : android.bluetooth.BluetoothGattServerCallback() {
  val bluetooth = bluetooth

  override fun onCharacteristicReadRequest (
    device: android.bluetooth.BluetoothDevice,
    requestId: Int,
    offset: Int,
    characteristic: android.bluetooth.BluetoothGattCharacteristic
  ) {
    console.log("onCharacteristicReadRequest")
    super.onCharacteristicReadRequest(device, requestId, offset, characteristic)
  }

  override fun onCharacteristicWriteRequest (
    device: android.bluetooth.BluetoothDevice,
    requestId: Int,
    characteristic: android.bluetooth.BluetoothGattCharacteristic,
    preparedWrite: Boolean,
    responseNeeded: Boolean,
    offset: Int,
    value: ByteArray
  ) {
    console.log("onCharacteristicWriteRequest")
    super.onCharacteristicWriteRequest(
      device,
      requestId,
      characteristic,
      preparedWrite,
      responseNeeded,
      offset,
      value
    )
  }

  override fun onConnectionStateChange (
    device: android.bluetooth.BluetoothDevice,
    status: Int,
    state: Int
  ) {
    if (state == android.bluetooth.BluetoothProfile.STATE_CONNECTED) {
      this.bluetooth.onDevice(device, true)
    } else if (this.bluetooth.gatts.contains(device.address)) {
      this.bluetooth.gatts.remove(device.address)
    }

    console.log("onConnectionStateChange($status, $state) device=${device.address}")

    super.onConnectionStateChange(device, status, state)
  }

  override fun onDescriptorReadRequest (
    device: android.bluetooth.BluetoothDevice,
    requestId: Int,
    offset: Int,
    descriptor: android.bluetooth.BluetoothGattDescriptor
  ) {
    console.log("onDescriptorReadRequest($requestId)")
    super.onDescriptorReadRequest(device, requestId, offset, descriptor)
  }

  override fun onDescriptorWriteRequest (
    device: android.bluetooth.BluetoothDevice,
    requestId: Int,
    descriptor: android.bluetooth.BluetoothGattDescriptor,
    preparedWrite: Boolean,
    responseNeeded: Boolean,
    offset: Int,
    value: ByteArray
  ) {
    console.log("onDescriptorWriteRequest($requestId, $preparedWrite, $responseNeeded)")
    super.onDescriptorWriteRequest(
      device,
      requestId,
      descriptor,
      preparedWrite,
      responseNeeded,
      offset,
      value
    )
  }

  override fun onExecuteWrite (
    device: android.bluetooth.BluetoothDevice,
    requestId: Int,
    execute: Boolean
  ) {
    super.onExecuteWrite(device, requestId, execute)
  }

  override fun onNotificationSent (
    device: android.bluetooth.BluetoothDevice,
    status: Int
  ) {
    console.log("onNotificationSent($status)")
    super.onNotificationSent(device, status)
  }

  override fun onServiceAdded (
    status: Int,
    service: android.bluetooth.BluetoothGattService
  ) {
    super.onServiceAdded(status, service)
    this.bluetooth.startScanning()
    this.bluetooth.handler.postDelayed({
      this.bluetooth.startAdvertising()
    }, 1000L)
  }
}

open class BluetoothCallbacks (bluetooth: Bluetooth) {
  val scan = BluetoothScanCallback(bluetooth)
  val advertise = BluetoothAdvertiseCallback(bluetooth)
  val gattServer = BluetoothGattServerServerCallback(bluetooth)
  val gattClient = BluetoothGattServerClientCallback(bluetooth)
}

open class Bluetooth (
  context: android.content.Context,
  runtime: Runtime,
  configuration: IBluetoothConfiguration
) {
  open protected val TAG = "Bluetooth"

  companion object {
    val REQUEST_ENABLE_BLUETOOTH = 1
    val PIN_DEFAULT_VALUE = "0000"
    val ADVERTISE_TIMEOUT = 30 * 1000L
    val SCAN_TIMEOUT = 30 * 10000L
  }

  val context = context
  val manager = configuration.manager
  val runtime = WeakReference(runtime)
  val callbacks = BluetoothCallbacks(this)

  val emit = configuration.emit
  val send = configuration.send

  // ble
  var pin = configuration.pin // TODO(jwerle): make this configurable from `socket.ini`
  val handler = runtime.handler
  var server: BluetoothGattServer? = null

  // peripherals
  val gatts = mutableMapOf<String, BluetoothGatt>()
  val queues = mutableMapOf<String, BluetoothCommandQueue>()

  // state predicates
  var isStarted = false
  var isScanning = false
  var isAdvertising = false

  fun isSupported (): Boolean {
    return this.manager.adapter != null
  }

  fun isEnabled (): Boolean {
    return this.manager.adapter?.isEnabled ?: false
  }

  fun start () {
    if (!this.isStarted) {
      val callback = callbacks.gattServer
      this.server = manager.openGattServer(context, callback)
      this.isStarted = true
      this.onStart()
    }
  }

  open fun onStart () {
    // noop
  }

  open fun onDevice (
    device: BluetoothDevice,
    isConnectable: Boolean = true,
    gatt: BluetoothGatt? = null
  ) {
    if (this.gatts.contains(device.address)) return

    val pin = this.pin
    val context = this.context
    val callback = this.callbacks.gattClient
    val transport = BluetoothDevice.TRANSPORT_LE
    val autoConnect = true

    device.apply {
      if (pin != null) {
        setPin(pin.toByteArray())
      }
    }

    if (gatt != null) {
      this.gatts[device.address] = gatt
    } else if (isConnectable) {
      this.gatts[device.address] = device.connectGatt(
        context,
        autoConnect,
        callback,
        transport
      )
    }

    this.getDeviceQueue(device.address)
  }

  fun addToDeviceQueue (address: String, function: BluetoothCommandFunction): BluetoothCommandQueue? {
    val queue = this.getDeviceQueue(address) ?: return null
    queue.push(function)
    return queue
  }

  fun getDeviceQueue (address: String): BluetoothCommandQueue? {
    val gatt = this.gatts[address] ?: return null
    val queue = this.queues[address] ?: BluetoothCommandQueue(this, gatt)

    if (this.queues[address] == null) {
      this.queues[address] = queue
    }

    return queue
  }

  fun addService (serviceId: String): BluetoothService {
    val server = this.server

    if (!this.isStarted || server == null) {
      throw Error("Bluetooth not started or ready")
    }

    val uuid = UUID.fromString(serviceId)

    if (server.getService(uuid) == null) {
      val service = BluetoothService(uuid)
      server.addService(service)
      return service
    }

    return server.getService(uuid) as BluetoothService
  }

  fun addCharacteristic (
    serviceId: String,
    characteristicId: String
  ) : BluetoothCharacteristic {
    val server = this.server

    if (!this.isStarted || server == null) {
      throw Error("Bluetooth not started or ready")
    }

    val uuid = UUID.fromString(characteristicId)
    val service = this.addService(serviceId)

    if (service.getCharacteristic(uuid) == null) {
      val characteristic = BluetoothCharacteristic(uuid)
      console.log("addCharacteristic")
      service.addCharacteristic(characteristic)
      return characteristic
    }

    return service.getCharacteristic(uuid) as BluetoothCharacteristic
  }

  fun startService (serviceId: String): Boolean {
    this.addService(serviceId)
    return true
  }

  fun startScanning (): Boolean {
    if (!isStarted || server?.services?.size == 0) return false
    if (isScanning) return true

    val server = this.server ?: return false
    val adapter = manager.adapter ?: return false
    val scanner = adapter.bluetoothLeScanner ?: return false
    val filters = mutableListOf<android.bluetooth.le.ScanFilter>()

    for (service in server.services) {
      console.log("start scanning for ${service.uuid.toString()}")
      filters.add(
        // https://developer.android.com/reference/android/bluetooth/le/ScanFilter.Builder
        android.bluetooth.le.ScanFilter.Builder()
        .apply {
          setServiceUuid(android.os.ParcelUuid(service.uuid))
        }
        .build()
      )
    }

    // https://developer.android.com/reference/android/bluetooth/le/ScanSettings.Builder
    val settings = android.bluetooth.le.ScanSettings.Builder()
      .apply {
        setNumOfMatches(android.bluetooth.le.ScanSettings.MATCH_NUM_MAX_ADVERTISEMENT)
        setCallbackType(android.bluetooth.le.ScanSettings.CALLBACK_TYPE_ALL_MATCHES)
        setMatchMode(android.bluetooth.le.ScanSettings.MATCH_MODE_AGGRESSIVE)
        setScanMode(android.bluetooth.le.ScanSettings.SCAN_MODE_BALANCED)
        setPhy(android.bluetooth.le.ScanSettings.PHY_LE_ALL_SUPPORTED)
      }
      .build()

    this.handler.postDelayed({
      if (this.isScanning) {
        this.stopScanning()
        this.isScanning = true
        this.handler.postDelayed({
          if (this.isScanning) {
            this.startScanning()
          }
        }, 500L)
      }
    }, SCAN_TIMEOUT)

    if (isScanning) {
      scanner.stopScan(this.callbacks.scan)
    }

    isScanning = true
    scanner.startScan(filters, settings, this.callbacks.scan)

    return true
  }

  fun stopScanning (): Boolean {
    if (!isScanning) return true
    val adapter = manager.adapter ?: return false
    val scanner = adapter.bluetoothLeScanner ?: return false
    isScanning = false
    scanner.stopScan(this.callbacks.scan)
    return true
  }

  fun startAdvertising (): Boolean {
    if (!isStarted || server?.services?.size == 0) return false
    if (isAdvertising) return true

    val server = this.server ?: return false
    val adapter = manager.adapter ?: return false
    val advertiser = adapter.bluetoothLeAdvertiser ?: return false
    val settings = android.bluetooth.le.AdvertiseSettings.Builder()
      .setAdvertiseMode(android.bluetooth.le.AdvertiseSettings.ADVERTISE_MODE_BALANCED)
      .build()

    var data = android.bluetooth.le.AdvertiseData.Builder()
      .setIncludeDeviceName(true)

    for (service in server.services) {
      val uuid = android.os.ParcelUuid(service.uuid)
      data.addServiceUuid(uuid)
    }

    this.handler.postDelayed({
      val wasAdvertising = isAdvertising
      this.stopAdvertising()
      if (wasAdvertising) {
        this.startAdvertising()
      }
    }, ADVERTISE_TIMEOUT)

    if (isAdvertising) {
      advertiser.stopAdvertising(this.callbacks.advertise)
    }

    isAdvertising = true
    advertiser.startAdvertising(
      settings,
      data.build(),
      this.callbacks.advertise
    )

    return true
  }

  fun stopAdvertising (): Boolean {
    if (!isAdvertising) return true
    val adapter = manager.adapter ?: return false
    val advertiser = adapter.bluetoothLeAdvertiser ?: return false
    isAdvertising = false
    advertiser.stopAdvertising(this.callbacks.advertise)
    return true
  }

  fun publish (
    serviceId: String,
    characteristicId: String,
    bytes: ByteArray? = null
  ): Boolean {
    val characteristicUUID = UUID.fromString(characteristicId)
    val serviceUUID = UUID.fromString(serviceId)
    val server = this.server ?: return false

    this.addService(serviceUUID.toString())
    this.addCharacteristic(
      serviceUUID.toString(),
      characteristicUUID.toString()
    )

    if (bytes == null) {
      return true
    }

    val localService = server.getService(serviceUUID)
    val localCharacteristics = localService?.characteristics

    if (localService != null && localCharacteristics != null) {
      console.log("not null locally")
    }

    for (entry in this.gatts) {
      val queue = this.getDeviceQueue(entry.value.device.address) ?: continue
      val service = entry.value.getService(serviceUUID) ?: continue
      var characteristic = service.getCharacteristic(characteristicUUID) ?:  continue

      console.log("preparing write op queue")

      queue.push({ gatt ->
        console.log("writeCharacteristic(${characteristic.uuid.toString()}, ${String(bytes)})")
        val status = gatt.writeCharacteristic(
          characteristic,
          bytes,
          android.bluetooth.BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
        )

        status
      })

      queue.push({ gatt ->
        console.log("setCharacteristicNotification(${characteristic.uuid.toString()}")
        gatt.setCharacteristicNotification(characteristic, true)
        BluetoothCommand.STATUS_SUCCESS
      })

      queue.push({ gatt ->
        console.log("notifyCharacteristicChanged(${gatt.device.address}, ${characteristic.uuid.toString()}, ${bytes.toString()})")
        if (0 == server.notifyCharacteristicChanged(gatt.device, characteristic, false, bytes)) {
          console.log("notifyCharacteristicChanged: STATUS_CONTINUE")
          BluetoothCommand.STATUS_CONTINUE
        } else {
          console.log("notifyCharacteristicChanged: STATUS_FAILURE")
          BluetoothCommand.STATUS_FAILURE
        }
      })

      for (descriptor in characteristic.descriptors) {
        queue.push({ gatt ->
          gatt.writeDescriptor(
            descriptor,
            android.bluetooth.BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
          )
        })
      }

      queue.next()
    }

    return true
  }

  fun subscribe (serviceId: String, characteristicId: String): Boolean {
    val characteristicUUID = UUID.fromString(characteristicId)
    val serviceUUID = UUID.fromString(serviceId)

    this.addService(serviceUUID.toString())
    this.addCharacteristic(
      serviceUUID.toString(),
      characteristicUUID.toString()
    )

    for (entry in this.gatts) {
      val queue = this.getDeviceQueue(entry.value.device.address) ?: return false
      queue.push({ gatt ->
        if (gatt.discoverServices()) {
          BluetoothCommand.STATUS_SUCCESS
        } else {
          BluetoothCommand.STATUS_FAILURE
        }
      })

      queue.next()
    }

    return this.startScanning()
  }
}
