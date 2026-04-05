#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
电机串口通信协议 - Python示例代码
用于下位机或测试工具
"""

import struct
import serial
import time
from typing import List, Tuple
import binascii

# CRC32查找表
CRC32_TABLE = [
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
]


def calculate_crc32(data: bytes) -> int:
    """计算CRC32校验值"""
    crc = 0xFFFFFFFF
    for byte in data:
        index = (crc ^ byte) & 0xFF
        crc = (crc >> 8) ^ CRC32_TABLE[index]
    return (~crc) & 0xFFFFFFFF


class MotorControlPacket:
    """电机控制数据包"""
    HEADER = b'\xFF\xAA'
    FORMAT = '<2sHBfff'  # 小端：帧头(2) + 长度(2) + ID(1) + 位置(4) + 速度(4) + 力矩(4)
    
    def __init__(self, motor_id: int, position: float, velocity: float, torque: float):
        self.motor_id = motor_id
        self.position = position
        self.velocity = velocity
        self.torque = torque
    
    @classmethod
    def parse(cls, data: bytes) -> 'MotorControlPacket':
        """解析控制数据包"""
        if len(data) < 21:
            raise ValueError("数据包长度不足")
        
        if data[0:2] != cls.HEADER:
            raise ValueError("帧头错误")
        
        # 解析数据（不包括CRC32）
        header, data_length, motor_id, position, velocity, torque = struct.unpack(
            cls.FORMAT, data[0:17]
        )
        
        # 验证CRC32
        received_crc = struct.unpack('<I', data[17:21])[0]
        calculated_crc = calculate_crc32(data[0:17])
        
        if received_crc != calculated_crc:
            raise ValueError(f"CRC32校验失败: 接收={received_crc:08X}, 计算={calculated_crc:08X}")
        
        return cls(motor_id, position, velocity, torque)
    
    def __str__(self):
        return (f"MotorControl(ID={self.motor_id}, "
                f"pos={self.position:.3f}, "
                f"vel={self.velocity:.3f}, "
                f"torque={self.torque:.3f})")


class MotorFeedbackPacket:
    """电机反馈数据包"""
    HEADER = b'\xFF\xAA'
    
    def __init__(self):
        self.motors = []
    
    def add_motor(self, motor_id: int, position: float, velocity: float, 
                  current: float, temperature: float, timestamp: int = 0):
        """添加一个电机的反馈数据"""
        self.motors.append({
            'id': motor_id,
            'position': position,
            'velocity': velocity,
            'current': current,
            'temperature': temperature,
            'timestamp': timestamp
        })
    
    def build(self) -> bytes:
        """构建反馈数据包"""
        motor_count = len(self.motors)
        data_length = motor_count * 21  # 每个电机21字节
        
        # 构建包头
        packet = struct.pack('<2sHB', self.HEADER, data_length, motor_count)
        
        # 添加每个电机的数据
        for motor in self.motors:
            motor_data = struct.pack('<BffffI',
                                    motor['id'],
                                    motor['position'],
                                    motor['velocity'],
                                    motor['current'],
                                    motor['temperature'],
                                    motor['timestamp'])
            packet += motor_data
        
        # 计算并添加CRC32
        crc32 = calculate_crc32(packet)
        packet += struct.pack('<I', crc32)
        
        return packet
    
    @classmethod
    def parse(cls, data: bytes) -> 'MotorFeedbackPacket':
        """解析反馈数据包"""
        if len(data) < 9:
            raise ValueError("数据包长度不足")
        
        if data[0:2] != cls.HEADER:
            raise ValueError("帧头错误")
        
        # 解析包头
        header, data_length, motor_count = struct.unpack('<2sHB', data[0:5])
        
        # 计算总长度
        total_length = 5 + data_length + 4
        if len(data) < total_length:
            raise ValueError(f"数据包不完整: 需要{total_length}字节, 实际{len(data)}字节")
        
        # 验证CRC32
        crc_offset = total_length - 4
        received_crc = struct.unpack('<I', data[crc_offset:crc_offset+4])[0]
        calculated_crc = calculate_crc32(data[0:crc_offset])
        
        if received_crc != calculated_crc:
            raise ValueError(f"CRC32校验失败: 接收={received_crc:08X}, 计算={calculated_crc:08X}")
        
        # 解析电机数据
        feedback = cls()
        offset = 5
        for i in range(motor_count):
            motor_id, position, velocity, current, temperature, timestamp = struct.unpack(
                '<BffffI', data[offset:offset+21]
            )
            feedback.add_motor(motor_id, position, velocity, current, temperature, timestamp)
            offset += 21
        
        return feedback
    
    def __str__(self):
        result = f"MotorFeedback(count={len(self.motors)})\n"
        for motor in self.motors:
            result += (f"  Motor {motor['id']}: "
                      f"pos={motor['position']:.3f}, "
                      f"vel={motor['velocity']:.3f}, "
                      f"cur={motor['current']:.3f}A, "
                      f"temp={motor['temperature']:.1f}°C\n")
        return result


class MotorSerialProtocol:
    """电机串口协议处理类"""
    
    def __init__(self, port: str, baudrate: int = 921600):
        self.serial = serial.Serial(port, baudrate, timeout=0.1)
        self.rx_buffer = bytearray()
    
    def send_control(self, motor_id: int, position: float, velocity: float, torque: float):
        """发送控制指令"""
        data_length = 1 + 3 * 4  # motor_id(1) + 3个float(12)
        
        # 构建数据包（不包括CRC32）
        packet = struct.pack('<2sHBfff',
                           b'\xFF\xAA',
                           data_length,
                           motor_id,
                           position,
                           velocity,
                           torque)
        
        # 计算并添加CRC32
        crc32 = calculate_crc32(packet)
        packet += struct.pack('<I', crc32)
        
        self.serial.write(packet)
        print(f"发送控制指令: Motor {motor_id}, pos={position:.3f}, vel={velocity:.3f}, torque={torque:.3f}")
    
    def send_feedback(self, motors: List[Tuple[int, float, float, float, float]]):
        """发送反馈数据（批量）"""
        feedback = MotorFeedbackPacket()
        timestamp = int(time.time() * 1000) % (2**32)  # 毫秒时间戳
        
        for motor_id, position, velocity, current, temperature in motors:
            feedback.add_motor(motor_id, position, velocity, current, temperature, timestamp)
        
        packet = feedback.build()
        self.serial.write(packet)
        print(f"发送反馈数据: {len(motors)}个电机")
    
    def receive_control(self) -> MotorControlPacket:
        """接收控制指令"""
        # 读取数据到缓冲区
        if self.serial.in_waiting > 0:
            self.rx_buffer.extend(self.serial.read(self.serial.in_waiting))
        
        # 查找帧头
        while len(self.rx_buffer) >= 21:
            if self.rx_buffer[0:2] == b'\xFF\xAA':
                try:
                    packet = MotorControlPacket.parse(bytes(self.rx_buffer[0:21]))
                    self.rx_buffer = self.rx_buffer[21:]
                    return packet
                except ValueError as e:
                    print(f"解析错误: {e}")
                    self.rx_buffer = self.rx_buffer[2:]
            else:
                self.rx_buffer = self.rx_buffer[1:]
        
        return None
    
    def receive_feedback(self) -> MotorFeedbackPacket:
        """接收反馈数据"""
        # 读取数据到缓冲区
        if self.serial.in_waiting > 0:
            self.rx_buffer.extend(self.serial.read(self.serial.in_waiting))
        
        # 查找帧头
        while len(self.rx_buffer) >= 9:
            if self.rx_buffer[0:2] == b'\xFF\xAA':
                # 解析数据长度
                data_length = struct.unpack('<H', self.rx_buffer[2:4])[0]
                total_length = 5 + data_length + 4
                
                if len(self.rx_buffer) >= total_length:
                    try:
                        packet = MotorFeedbackPacket.parse(bytes(self.rx_buffer[0:total_length]))
                        self.rx_buffer = self.rx_buffer[total_length:]
                        return packet
                    except ValueError as e:
                        print(f"解析错误: {e}")
                        self.rx_buffer = self.rx_buffer[2:]
                else:
                    break  # 等待更多数据
            else:
                self.rx_buffer = self.rx_buffer[1:]
        
        return None
    
    def close(self):
        """关闭串口"""
        self.serial.close()


# ========== 测试代码 ==========

def test_crc32():
    """测试CRC32计算"""
    test_data = b'\xFF\xAA\x0D\x00\x01\x00\x00\x00\x00'
    crc = calculate_crc32(test_data)
    print(f"CRC32测试: 0x{crc:08X}")


def test_control_packet():
    """测试控制数据包"""
    print("\n=== 测试控制数据包 ===")
    
    # 构建数据包
    data_length = 1 + 3 * 4
    packet = struct.pack('<2sHBfff',
                        b'\xFF\xAA',
                        data_length,
                        1,  # motor_id
                        1.57,  # position
                        0.5,  # velocity
                        0.1)  # torque
    crc32 = calculate_crc32(packet)
    packet += struct.pack('<I', crc32)
    
    print(f"数据包长度: {len(packet)}字节")
    print(f"数据包内容: {binascii.hexlify(packet)}")
    
    # 解析数据包
    parsed = MotorControlPacket.parse(packet)
    print(f"解析结果: {parsed}")


def test_feedback_packet():
    """测试反馈数据包"""
    print("\n=== 测试反馈数据包 ===")
    
    # 构建反馈包（3个电机）
    feedback = MotorFeedbackPacket()
    feedback.add_motor(1, 1.57, 0.5, 2.3, 45.5, 12345)
    feedback.add_motor(2, 0.78, 0.3, 1.8, 42.0, 12345)
    feedback.add_motor(3, 2.35, 0.8, 3.1, 48.2, 12345)
    
    packet = feedback.build()
    print(f"数据包长度: {len(packet)}字节")
    print(f"数据包内容: {binascii.hexlify(packet)}")
    
    # 解析数据包
    parsed = MotorFeedbackPacket.parse(packet)
    print(f"解析结果:\n{parsed}")


def test_serial_communication():
    """测试串口通信（需要实际硬件）"""
    print("\n=== 测试串口通信 ===")
    
    try:
        # 创建协议实例（根据实际串口修改）
        protocol = MotorSerialProtocol('/dev/ttyUSB0', 921600)
        
        # 发送控制指令
        protocol.send_control(motor_id=1, position=1.57, velocity=0.5, torque=0.1)
        
        # 发送反馈数据
        motors_data = [
            (1, 1.57, 0.5, 2.3, 45.5),
            (2, 0.78, 0.3, 1.8, 42.0),
            (3, 2.35, 0.8, 3.1, 48.2)
        ]
        protocol.send_feedback(motors_data)
        
        # 接收数据
        time.sleep(0.1)
        control_packet = protocol.receive_control()
        if control_packet:
            print(f"接收到控制指令: {control_packet}")
        
        feedback_packet = protocol.receive_feedback()
        if feedback_packet:
            print(f"接收到反馈数据:\n{feedback_packet}")
        
        protocol.close()
        
    except serial.SerialException as e:
        print(f"串口错误: {e}")
        print("提示: 请修改串口号后重试")


if __name__ == '__main__':
    print("电机串口通信协议测试\n")
    
    test_crc32()
    test_control_packet()
    test_feedback_packet()
    
    # 如果有实际硬件，取消下面的注释
    # test_serial_communication()
    
    print("\n测试完成!")
