# Heart beat and SpO2 detection

Máy đo nhịp tim và nồng độ SpO2 trong máu.

## Linh kiện

- Arduino UNO R3

- SparkFun MAX30102 BLACK

- Màn hình OLED 128x32

## Tại sao

Mục đích chính của repository này là hi vọng sẽ giúp được ai đó sau này khi
muốn tự làm ý tưởng tương tự bởi vì code có sẵn của [SparkFun](https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library/tree/master/examples)
khá là ấm ớ theo trải nghiệm của mình.

## Những điều đáng chú ý

Để tự làm một sản phẩm giống mình, bạn cần tìm hiểu:

- DSP Filter
- Thuật toán tìm đỉnh

## Phương thức hoạt động

Cảm biến có hai kênh IR và R, với mỗi kênh là một tín hiệu gồm hai bộ phận
là AC (Bộ phận thay đổi do tim bơm máu tới) và DC (Bộ phận không/ít thay đổi,
tồn tại do mô, da và các yếu tố bên ngoài ảnh hưởng)

### Tính nhịp tim

Sử dụng bộ lọc để tách phần AC của IR ra và sau đó áp dụng thuật toán tìm
đỉnh tìm ra deltaT là khoảng thời gian giữa hai đỉnh, lấy cái đó áp dụng công thức:

$$BPM = \frac{60000}{\Delta T}$$

### Tính $SpO_2$

Sử dụng bộ lọc tách phần AC và DC của cả IR và R ra, sau đó ráp vào công thức:

$$R = \frac{AC_{red} \div DC_{red}}{AC_{IR} \div DC_{IR}}$$

$$SpO_2 = 1.5958422 \times R^2 - 34.6596622 \times R + 112.6896759$$

## Về code trong repository này

Mình sử dụng các thư viện ngoài thay vì tự viết hết chủ yếu vì mình có biết làm
~~đéo~~ đâu. Thực sự, thứ quan trọng là biết cách sử dụng các cái *DSP Filter* với
*Tìm đỉnh* nếu trên để xử lý tín hiệu đọc từ cảm biến. Sau đó chỉ cần áp dụng
công thức là ra được SpO2 và nhịp tim.

Đồng thời mình không sử dụng phương pháp ghi nhận nhiều giá trị và tính ra trung bình
bởi vì ~~mình bị lười~~ cái bộ nhớ hihi.

*Nhìn lại thì thư viện Filter được sử dụng trong dự án này thật sự không phù hợp
vì để lọc tín hiệu cho tốt cần một bộ lọc bậc 4 trong khi bộ lọc trong thư viện
Filter chỉ có bậc 1. Nếu chồng bộ lọc lên thì nó ngốn tận 0.4Kb (?!). Vì mình
sử dụng thư viện Adafruit để điều khiển cái màn hình và nó cần 0.1Kb để hiển thị
với độ phân giải 128x32 nên bộ nhớ khá là chật chội. Nếu có ai tham khảo người đi
trước này làm ơn tự viết một cái DSP Filter riêng (Cụ thể là BiQuad). Nói vậy thôi,
chứ thực sự khi đo mình không thấy ảnh hưởng quá nhiều*

## Kết thúc

***Thanks for dropping by! From VNU-HCMUS with luv ❤️🧡💛💚💙💜.***
