#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"

#include <Filters.h>
#include <PeakDetection.h>

// Các bộ phận phần cứng
MAX30105 particleSensor; // Cảm biến
Adafruit_SSD1306 display(128, 32, &Wire, -1); // Màn hình
// Hình ảnh trái tim để màn hình trở nên sang trọng hơn
static const unsigned char PROGMEM logo3_bmp[] = { 
    0x01, 0xF0, 0x0F, 0x80, 0x06, 0x1C, 0x38, 0x60, 0x18, 0x06, 0x60, 0x18, 0x10, 0x01, 0x80, 0x08,
    0x20, 0x01, 0x80, 0x04, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x02, 0xC0, 0x00, 0x08, 0x03,
    0x80, 0x00, 0x08, 0x01, 0x80, 0x00, 0x18, 0x01, 0x80, 0x00, 0x1C, 0x01, 0x80, 0x00, 0x14, 0x00,
    0x80, 0x00, 0x14, 0x00, 0x80, 0x00, 0x14, 0x00, 0x40, 0x10, 0x12, 0x00, 0x40, 0x10, 0x12, 0x00,
    0x7E, 0x1F, 0x23, 0xFE, 0x03, 0x31, 0xA0, 0x04, 0x01, 0xA0, 0xA0, 0x0C, 0x00, 0xA0, 0xA0, 0x08,
    0x00, 0x60, 0xE0, 0x10, 0x00, 0x20, 0x60, 0x20, 0x06, 0x00, 0x40, 0x60, 0x03, 0x00, 0x40, 0xC0,
    0x01, 0x80, 0x01, 0x80, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0C, 0x00,
    0x00, 0x08, 0x10, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x01, 0x80, 0x00  
};

// Các bộ phận phần mềm
PeakDetection peakDetector; // Thư viện nhận diện đỉnh sóng

// Các bộ lọc, gọi F là tần số ta cho khi ta khởi tạo bộ lọc, ta có
// LPF: Low-pass filter, bộ lọc thông thấp: cho phép tần số dưới F đi qua, chặn tần số trên
// HPF: High-pass filter, bộ lọc thông cao: cho phép tần số trên F đi qua, chặn tần số dưới

// Bộ lọc tín hiệu IR
FilterOnePole irLPF_AC  = FilterOnePole(LOWPASS, 4);    // LPF ở đây đóng vai trò làm "mượt" tín hiệu, loại bỏ các thay đổi quá bất chợt
FilterOnePole irHPF_AC  = FilterOnePole(HIGHPASS, 0.5); // HPF ở đây đóng vai trò xóa bỏ thành phần DC của tín hiệu, chỉ giữ lại AC
FilterOnePole irLPF_DC  = FilterOnePole(LOWPASS, 0.5);  // LPF ở đây đóng vai trò xóa bỏ thành phần AC của tín hiệu, chỉ giữ lại DC

// Bộ lọc tín hiệu R
FilterOnePole rLPF_AC   = FilterOnePole(LOWPASS, 4);
FilterOnePole rHPF_AC   = FilterOnePole(HIGHPASS, 0.5);
FilterOnePole rLPF_DC   = FilterOnePole(LOWPASS, 0.5);

int peak1T      = 0;        // Thời gian nhịp 1 xảy ra
int peak2T      = 0;        // Thời gian nhịp 2 xảy ra
bool peakCycle  = false;    // Lưu trữ xem nhịp 1 đã xảy ra chưa 

byte bpm        = 0;        // Nhịp tim
float spo2      = 0;        // Nồng độ SpO2

float irRMS         = 0;    // Giá trị RMS (hiểu nôm na là trung bình) của tín hiệu AC của IR
float rRMS          = 0;    // Giá trị RMS (hiểu nôm na là trung bình) của tín hiệu AC của R
float irAverage_DC  = 0;    // Giá trị trung bình của tín hiệu DC của IR
float rAverage_DC   = 0;    // Giá trị trung bình của tín hiệu DC của R

// Các biến này hỗ trợ tính toán cho 4 giá trị trên
float irSum_AC      = 0;
float rSum_AC       = 0;
float irSum_DC      = 0;
float rSum_DC       = 0;

void setup() {
    Serial.begin(19200); // In tùm lum tùm la ra Serial Monitor

    // Khởi tạo màn hình
    // Thực hiện trước vì cái màn hình quái này cần 1Kb RAM và em đã phải chiến đấu với nó
    // nguyên một ngày để nó hiển thị
    display.begin(SSD1306_SWITCHCAPVCC,  0x3C);
    display.display();
    delay(3000);
    display.clearDisplay();
    display.display();

    // Khởi tạo cảm biến
    particleSensor.begin(Wire, I2C_SPEED_FAST);
    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x0A);

    // Khởi tạo cái lớp nhận diện đỉnh tín hiệu gì đó
    peakDetector.begin(20, 1, 0.5);  
}

void loop() {
    // Đọc giá trị
    float irValue = (float)particleSensor.getIR();
    float rValue = (float)particleSensor.getRed();

    // Lọc
    irHPF_AC.input(irValue);
    irLPF_AC.input(irHPF_AC.output());
    irLPF_DC.input(irValue);

    rHPF_AC.input(rValue);
    rLPF_AC.input(rHPF_AC.output());
    rLPF_DC.input(rValue);

    // Lấy kết quả từ các bộ lọc, lưu ý kết quả này có chút delay so với giá trị đọc thật sự
    float irAC = irLPF_AC.output();
    float irDC = irLPF_DC.output();

    float rAC = rLPF_AC.output();
    float rDC = rLPF_DC.output();


    irSum_AC    += abs(irAC);
     rSum_AC     += abs(rAC);

    irSum_DC    += abs(irDC);
    rSum_DC     += abs(rDC);

    // Nhận diện đỉnh sóng phần AC của tín hiệu IR
    // peak có giá trị như sau:
    //      0: không có sóng
    //      1: đỉnh
    //      -1: trũng
    peakDetector.add(irAC);
    int peak = peakDetector.getPeak();
    int currentTime = millis();
    // Vì một lí do nào đó siêu bí ẩn, thư viện peakDetector nhận diện trũng tốt hơn là đỉnh sóng
    // nên việc đo đạc thời gian sẽ được thực hiện khi nó phát hiện ra hai trũng liên tiếp
    // hay nói cách khác là peakCycle = true
    switch(peak) {
        // Trũng
        case -1:
            // Bắt đầu nhịp thứ 1
            if (!peak1T) {
                irSum_AC    = 0;
                irSum_DC    = 0;
                rSum_AC     = 0;
                rSum_DC     = 0;
                peak1T      = currentTime;
            }

            // Bắt đầu nhịp thứ 2
            if (peakCycle && !peak2T) {
                peak2T = currentTime;
            }
            break;
        case 0:
            // Nếu đã ghi nhận nhịp 1 nhưng chưa ghi nhận nhịp 2 -> Bắt đầu ghi nhận nhịp 2
            if (peak1T && !peak2T) {
                peakCycle = true;
            
            // Nếu đã ghi nhận cả nhịp 1 và nhịp 2 -> Bắt đầu tính toán
            } else if (peak1T && peak2T && peakCycle) {
                float deltaT = (float)peak2T - (float)peak1T; // Đơn vị Millis
                // Nhịp tim
                bpm     = 60000 / (peak2T - peak1T);

                // Giá trị RMS của tín hiệu AC của IR và của R
                irRMS   = sqrt((1.0 / (100.0 * ((deltaT / 1000.0))) * irSum_AC * irSum_AC));
                rRMS    = sqrt((1.0 / (100.0 * ((deltaT / 1000.0))) * rSum_AC * rSum_AC));

                // Giá trị trung bình của tín hiệu DC của IR và của R
                irAverage_DC    = (irSum_DC / (100.0 * deltaT));
                rAverage_DC     = (rSum_DC / (100.0 * deltaT));

                // Reset mọi thứ
                peak1T      = 0;
                peak2T      = 0;
                peakCycle   = false;
            }
            break;
        case 1:
        default: 
            break;
    }

    // Kiểm tra xem các giá trị này có khác không hay không để
    // tránh tình trạng chia cho 0 và làm nổ Arduino
    if (irRMS && rRMS && irDC && rDC) {
        // Biểu thức rơi từ trời xuống này chính là công thức tính nồng độ SpO2
        float R = (rRMS / rAverage_DC) / (irRMS / irAverage_DC);
        spo2 = 1.5958422 * R * R - 34.6596622 * R + 112.6896759;
    }

    // Giữ SpO2 dưới mức 100 vì nếu nó trên 100 thì nó
    // không có hợp lí lắm
    spo2 = (spo2 > 100.0) ? 100 : spo2;

    display.clearDisplay();
    display.drawBitmap(0, 0, logo3_bmp, 32, 32, WHITE);
    display.setTextSize(1);
    display.setTextColor(WHITE);             
    display.setCursor(50, 0);                
    display.println("BPM");             
    display.setCursor(50, 18);                
    display.println(bpm);
    display.setCursor(90, 0); 
    display.println("SpO2");             
    display.setCursor(90, 18);                
    display.println(spo2);  
    display.display();

    // In các thứ ra Serial Monitor để giám sát và đánh đòn nếu nó không chịu nghe lời
    Serial.print("irValue:");
    Serial.print(irValue);
    Serial.print(",");

    Serial.print("IR_AC:");
    Serial.print(irAC);
    Serial.print(",");

    Serial.print("IR_DC:");
    Serial.print(irDC);
    Serial.print(",");

     Serial.print("R_AC:");
    Serial.print(rAC);
    Serial.print(",");

    Serial.print("R_DC:");
    Serial.print(rDC);
    Serial.print(",");

    Serial.print("Peak:");
    Serial.print(peak);
    Serial.print(",");

    Serial.print("IR_RMS:");
    Serial.print(irRMS);
    Serial.print(",");

    Serial.print("R_RMS:");
    Serial.print(rRMS);
    Serial.print(",");

    Serial.print("SpO2:");
    Serial.print(spo2);
    Serial.print(",");

    Serial.print("BPM:");
    Serial.print(bpm);
    Serial.println();
}
