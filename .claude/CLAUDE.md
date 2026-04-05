# 3D_MVP - Vulkan 3D 렌더러

## 프로젝트
- Vulkan 기반 3D 렌더러
- C++20, CMake + vcpkg

## 코딩 규칙
- 전방선언은 인라인으로 할 것
- 네이밍 접미사 (멤버 변수에만 적용, 지역변수에는 붙이지 않음):
  - 인스턴스 소유: `Inst` (예: `SwapChainInst`)
  - 참조: `Ref` (예: `DeviceRef`)
  - 인터페이스: `Interface` (예: `RenderableInterface`)
- 변수명 축약 금지, 풀네임으로 작성
- 함수명은 카멜케이스 (예: `initVulkan`, `createInstance`, `mainLoop`)
- 클래스명은 파스칼케이스 (예: `QWaveEngine`, `VulkanDevice`)
- 지역변수는 카멜케이스 (예: `movementVector`)
- 지역 포인터 변수는 `p` 접두사 + 파스칼케이스 (예: `pPhysicalDevice`)
- 멤버 변수는 캡슐화 (private/protected), 외부 접근은 반드시 Getter/Setter로
- 작업 완료 후 반드시 헤더 include 누락/중복 없는지 재확인할 것
- 포인터 사용 시 반드시 널가드할 것. 예외 없음
- 널가드는 assert 또는 ENSURE 사용, 얼리리턴 허용
- 상속받는 클래스의 소멸자는 반드시 `virtual` 붙일 것

## Git 규칙
- 커밋 전 반드시 유저 확인 받을 것
- 커밋 메시지에 Co-Authored-By 절대 넣지 말 것
- Git LFS 설정 확인 후 커밋할 것

## 작업 규칙
- 유저가 지시한 것만 수행할 것, 임의로 추가 작업 금지
- 절대 스스로 추론/판단하지 말 것. 판단이 필요하면 반드시 유저에게 물어볼 것
