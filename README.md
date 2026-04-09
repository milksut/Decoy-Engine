# Decoy-Engine
**Gerekli kütüphaneler**

- Stb_image -> Görselleri (PNG, JPG vb.) kolayca yüklemek için

- GLFW -> Pencere oluşturma, giriş (klavye, fare) ve OpenGL context yönetimi

- GLM -> OpenGL için matematik kütüphanesi (vektör, matris, dönüşümler)

- GLAD -> OpenGL fonksiyonlarının yüklenmesi

- ASSİMP -> Dışarıdan modelleri yüklemeye yarayan kütüphane

- X11 -> Linux kullanıcılarının kurması gerekiyor

- After the initial download take out of the comment this line in CMakelists.txt
  
"set(FETCHCONTENT_FULLY_DISCONNECTED ON)"
