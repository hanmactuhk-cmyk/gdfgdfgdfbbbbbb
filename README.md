# HNStudio Musik AI - Phần Mềm Hát LIVE Chuyên Nghiệp Trên Máy Tính

**Tác giả:** Hoài Nguyễn Studio  
**Zalo hỗ trợ:** 0965.043.000  
**Hệ điều hành mục tiêu:** Windows 10 / Windows 11 (64-bit)  
**Công nghệ:** C++17, JUCE Framework 7.0.12, CMake 3.22+, Native VST3 Hosting

---

## 1. GIỚI THIỆU & CẤU TRÚC 2-BUS
- **MIC BUS:** Mic In -> Noise Gate -> Compressor -> 13-Band Parametric EQ -> De-Esser -> Reverb -> VST3 Insert Rack -> Mic Master
- **MUSIC BUS:** Beat File / SFX -> Music Volume -> Music Master
- **MASTER BUS:** Mic Master + Music Master -> Master Limiter -> Audio Output
- Beat nhạc chạy trên bus riêng biệt, TUYỆT ĐỐI không bị dính hiệu ứng của Mic.

## 2. CÁCH BUILD BẰNG GITHUB ACTIONS
1. Đẩy toàn bộ thư mục này lên GitHub repo của bạn.
2. Vào tab **Actions** trên GitHub.
3. Sau 3-5 phút, vào mục **Artifacts** tải file **HNStudio-Musik-AI-Windows-x64.zip** có sẵn file **HNStudioMusikAI.exe** để sử dụng ngay!

## 3. CÀI ĐẶT & SCAN VST3
- Vào mục **CÀI ĐẶT VST** -> Bấm **SCAN VST** để quét toàn bộ plugin trên máy tính.
- Các đường dẫn mặc định:
  - C:\Program Files\Common Files\VST3
  - C:\Program Files\VSTPlugins
  - C:\Program Files\Steinberg\VstPlugins
- Hỗ trợ Auto-Tune, FabFilter, Waves, iZotope, Valhalla...
