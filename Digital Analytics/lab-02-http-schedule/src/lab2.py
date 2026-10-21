import requests


def get_room_schedule(building_name: str, room_name: str, date: str):
    # 1. Получаем список корпусов
    buildings_url = "https://ruz.spbstu.ru/api/v1/ruz/buildings"
    response = requests.get(buildings_url)
    data = response.json()

    building_id = None
    for building in data.get("buildings", []):
        if isinstance(building, dict) and building.get("name") == building_name:
            building_id = building.get("id")
            break

    if building_id is None:
        return None

    # 2. Получаем список аудиторий корпуса
    rooms_url = f"https://ruz.spbstu.ru/api/v1/ruz/buildings/{building_id}/rooms"
    response = requests.get(rooms_url)
    data = response.json()

    room_id = None
    for room in data.get("rooms", []):
        if isinstance(room, dict) and room.get("name") == room_name:
            room_id = room.get("id")
            break

    if room_id is None:
        return None

    # 3. Получаем расписание аудитории на нужную неделю
    schedule_url = f"https://ruz.spbstu.ru/api/v1/ruz/buildings/{building_id}/rooms/{room_id}/scheduler"
    response = requests.get(schedule_url, params={"date": date})
    schedule = response.json()

    result = []

    for day in schedule.get("days", []):
        if day.get("date") == date:
            for lesson in day.get("lessons", []):
                time_start = lesson.get("time_start") or "None"
                time_end = lesson.get("time_end") or "None"
                subject = lesson.get("subject") or "None"
                lesson_type = (lesson.get("typeObj") or {}).get("name") or "None"

                teachers = lesson.get("teachers") or []
                teacher_names = ",".join(
                    teacher.get("full_name")
                    for teacher in teachers
                    if isinstance(teacher, dict) and teacher.get("full_name")
                ) or "None"

                groups = lesson.get("groups") or []
                group_names = ",".join(
                    group.get("name")
                    for group in groups
                    if isinstance(group, dict) and group.get("name")
                ) or "None"

                auditories = lesson.get("auditories") or []
                place_names = ",".join(
                    aud.get("name")
                    for aud in auditories
                    if isinstance(aud, dict) and aud.get("name")
                ) or "None"

                result.append(f"Time:{time_start}-{time_end}")
                result.append(f"Subject:{subject}")
                result.append(f"Type:{lesson_type}")
                result.append(f"Teacher:{teacher_names}")
                result.append(f"Groups:{group_names}")
                result.append(f"Place:{place_names}")

    if not result:
        return None

    return "\n".join(result) + "\n"


def get_teacher_schedule(teacher_name: str, date: str):
    surname = teacher_name.split()[0]

    # Поиск преподавателя
    search_url = "https://ruz.spbstu.ru/api/v1/ruz/search/teachers"
    response = requests.get(search_url, params={"q": surname})
    data = response.json()

    teacher_id = None
    for teacher in data.get("teachers", []):
        if isinstance(teacher, dict) and teacher.get("full_name") == teacher_name:
            teacher_id = teacher.get("id")
            break

    if teacher_id is None:
        return None

    # Получение расписания на нужную неделю
    schedule_url = f"https://ruz.spbstu.ru/api/v1/ruz/teachers/{teacher_id}/scheduler"
    response = requests.get(schedule_url, params={"date": date})
    schedule = response.json()

    result = []

    for day in schedule.get("days", []):
        if day.get("date") == date:
            for lesson in day.get("lessons", []):
                time_start = lesson.get("time_start") or "None"
                time_end = lesson.get("time_end") or "None"
                subject = lesson.get("subject") or "None"
                lesson_type = (lesson.get("typeObj") or {}).get("name") or "None"

                teachers = lesson.get("teachers") or []
                teacher_names = ",".join(
                    teacher.get("full_name")
                    for teacher in teachers
                    if isinstance(teacher, dict) and teacher.get("full_name")
                ) or "None"

                groups = lesson.get("groups") or []
                group_names = ",".join(
                    group.get("name")
                    for group in groups
                    if isinstance(group, dict) and group.get("name")
                ) or "None"

                auditories = lesson.get("auditories") or []
                place_names = ",".join(
                    aud.get("name")
                    for aud in auditories
                    if isinstance(aud, dict) and aud.get("name")
                ) or "None"

                result.append(f"Time:{time_start}-{time_end}")
                result.append(f"Subject:{subject}")
                result.append(f"Type:{lesson_type}")
                result.append(f"Teacher:{teacher_names}")
                result.append(f"Groups:{group_names}")
                result.append(f"Place:{place_names}")

    if not result:
        return None

    return "\n".join(result) + "\n"


def get_group_schedule(group_name: str, date: str):
    # Поиск группы
    search_url = "https://ruz.spbstu.ru/api/v1/ruz/search/groups"
    response = requests.get(search_url, params={"q": group_name})
    data = response.json()

    group_id = None
    for group in data.get("groups", []):
        if isinstance(group, dict) and group.get("name") == group_name:
            group_id = group.get("id")
            break

    if group_id is None:
        return None

    # Получение расписания на нужную неделю
    schedule_url = f"https://ruz.spbstu.ru/api/v1/ruz/scheduler/{group_id}"
    response = requests.get(schedule_url, params={"date": date})
    schedule = response.json()

    result = []

    for day in schedule.get("days", []):
        if day.get("date") == date:
            for lesson in day.get("lessons", []):
                time_start = lesson.get("time_start") or "None"
                time_end = lesson.get("time_end") or "None"
                subject = lesson.get("subject") or "None"
                lesson_type = (lesson.get("typeObj") or {}).get("name") or "None"

                teachers = lesson.get("teachers") or []
                teacher_names = ",".join(
                    teacher.get("full_name")
                    for teacher in teachers
                    if isinstance(teacher, dict) and teacher.get("full_name")
                ) or "None"

                groups = lesson.get("groups") or []
                group_names = ",".join(
                    group.get("name")
                    for group in groups
                    if isinstance(group, dict) and group.get("name")
                ) or "None"

                auditories = lesson.get("auditories") or []
                place_names = ",".join(
                    aud.get("name")
                    for aud in auditories
                    if isinstance(aud, dict) and aud.get("name")
                ) or "None"

                result.append(f"Time:{time_start}-{time_end}")
                result.append(f"Subject:{subject}")
                result.append(f"Type:{lesson_type}")
                result.append(f"Teacher:{teacher_names}")
                result.append(f"Groups:{group_names}")
                result.append(f"Place:{place_names}")

    if not result:
        return None

    return "\n".join(result) + "\n"

