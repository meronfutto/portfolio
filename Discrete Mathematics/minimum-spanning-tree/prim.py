import sys


def verify_mst(weight_matrix, total_weight, mst_edges):
    """
    Сравнивает наш MST с результатом networkx.
    """
    try:
        import networkx as nx

        n = len(weight_matrix)
        G = nx.Graph()

        # Добавляем рёбра с весами
        for i in range(n):
            for j in range(i + 1, n):
                if weight_matrix[i][j] != float('inf'):
                    G.add_edge(i, j, weight=weight_matrix[i][j])

        # Находим MST всех компонент с помощью networkx
        # Для каждой компоненты связности отдельно
        nx_weight = 0
        nx_edges = set()

        for component in nx.connected_components(G):
            subgraph = G.subgraph(component)
            mst_nx = nx.minimum_spanning_tree(subgraph, algorithm='prim')
            nx_weight += sum(data['weight'] for _, _, data in mst_nx.edges(data=True))
            for u, v in mst_nx.edges():
                nx_edges.add(tuple(sorted([u, v])))

        # Наши рёбра
        our_edges = set()
        for u, v, _ in mst_edges:
            our_edges.add(tuple(sorted([u, v])))

        # Вывод сравнения
        print("\n=== СРАВНЕНИЕ С NETWORKX ===")
        print(f"Наш вес MST:      {total_weight}")
        print(f"NetworkX вес MST: {nx_weight}")
        print(f"Веса совпадают:   {'ДА' if abs(nx_weight - total_weight) < 1e-9 else ' НЕТ'}")
        print(f"\nНаших рёбер:      {len(our_edges)}")
        print(f"NetworkX рёбер:   {len(nx_edges)}")
        print(f"Рёбра совпадают:  {'ДА' if our_edges == nx_edges else ' НЕТ'}")

        if our_edges != nx_edges:
            print(f"\nРёбра только у нас:     {our_edges - nx_edges}")
            print(f"Рёбра только у NetworkX: {nx_edges - our_edges}")

        return nx_weight == total_weight and our_edges == nx_edges

    except ImportError:
        print("\n  networkx не установлен. Установите: pip install networkx")
        return None

def read_matrix_from_file(filename):
    """
    Читает матрицу весов из файла.
    Формат: числа, разделённые пробелами, по строкам.
    0 означает отсутствие ребра (заменяется на inf).
    """
    matrix = []
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                row = []
                for val in line.split():
                    num = float(val)
                    row.append(num)
                matrix.append(row)
    except FileNotFoundError:
        print(f"Ошибка: файл '{filename}' не найден.")
        sys.exit(1)
    except ValueError as e:
        print(f"Ошибка при чтении числа: {e}")
        sys.exit(1)

    # Проверяем, что матрица квадратная
    n = len(matrix)
    for i, row in enumerate(matrix):
        if len(row) != n:
            print(f"Ошибка: строка {i + 1} имеет длину {len(row)}, ожидается {n}")
            sys.exit(1)
        # Заменяем 0 на inf, кроме диагонали
        for j in range(n):
            if i != j and matrix[i][j] == 0:
                matrix[i][j] = float('inf')

    return matrix


def prim_mst(weight_matrix, start_vertex=10):
    """
    Находит минимальное остовное дерево по алгоритму Прима.

    Args:
        weight_matrix: квадратная матрица весов
        start_vertex: индекс стартовой вершины (по умолчанию 10, что соответствует вершине 11)

    Returns:
        tuple: (общий вес MST, список рёбер в формате (u, v, weight), матрица смежности MST)
    """
    n = len(weight_matrix)
    if n == 0:
        return 0, [], []

    visited = [False] * n
    min_edge = [(float('inf'), -1)] * n
    min_edge[start_vertex] = (0, -1)  # Начинаем с указанной вершины

    total_weight = 0
    mst_edges = []

    # Матрица смежности для MST (изначально все 0)
    mst_adj_matrix = [[0] * n for _ in range(n)]

    for _ in range(n):
        u = -1
        min_weight = float('inf')

        for i in range(n):
            if not visited[i] and min_edge[i][0] < min_weight:
                min_weight = min_edge[i][0]
                u = i

        if u == -1:
            break

        visited[u] = True
        total_weight += min_weight

        if min_edge[u][1] != -1:
            v = min_edge[u][1]
            mst_edges.append((v, u, min_weight))
            # Заполняем матрицу смежности (неориентированный граф)
            mst_adj_matrix[u][v] = 1
            mst_adj_matrix[v][u] = 1

        for v in range(n):
            if not visited[v] and weight_matrix[u][v] < min_edge[v][0]:
                min_edge[v] = (weight_matrix[u][v], u)

    return total_weight, mst_edges, mst_adj_matrix


def prim_mst_all_components(weight_matrix, start_vertex=10):
    """
    Находит минимальное остовное дерево (лес) для ВСЕХ компонент связности.
    Начинает с указанной вершины, затем обрабатывает остальные компоненты.
    """
    n = len(weight_matrix)
    if n == 0:
        return 0, [], []

    visited = [False] * n
    mst_edges = []
    total_weight = 0
    mst_adj_matrix = [[0] * n for _ in range(n)]

    # Сначала запускаем Прим для компоненты, содержащей start_vertex
    comp_weight, comp_edges = prim_for_component(weight_matrix, start_vertex, visited)
    total_weight += comp_weight
    mst_edges.extend(comp_edges)

    for u, v, _ in comp_edges:
        mst_adj_matrix[u][v] = 1
        mst_adj_matrix[v][u] = 1

    # Затем для остальных непосещённых вершин
    for start in range(n):
        if not visited[start]:
            comp_weight, comp_edges = prim_for_component(weight_matrix, start, visited)
            total_weight += comp_weight
            mst_edges.extend(comp_edges)

            for u, v, _ in comp_edges:
                mst_adj_matrix[u][v] = 1
                mst_adj_matrix[v][u] = 1

    return total_weight, mst_edges, mst_adj_matrix


def prim_for_component(weight_matrix, start_vertex, visited):
    """
    Алгоритм Прима для одной компоненты связности.
    """
    n = len(weight_matrix)

    comp_visited = [False] * n
    min_edge = [(float('inf'), -1)] * n
    min_edge[start_vertex] = (0, -1)

    comp_weight = 0
    comp_edges = []

    for _ in range(n):
        u = -1
        min_weight = float('inf')

        for i in range(n):
            if not comp_visited[i] and not visited[i] and min_edge[i][0] < min_weight:
                min_weight = min_edge[i][0]
                u = i

        if u == -1:
            break

        comp_visited[u] = True
        visited[u] = True
        comp_weight += min_weight

        if min_edge[u][1] != -1:
            comp_edges.append((min_edge[u][1], u, min_weight))

        for v in range(n):
            if not comp_visited[v] and not visited[v] and weight_matrix[u][v] < min_edge[v][0]:
                min_edge[v] = (weight_matrix[u][v], u)

    return comp_weight, comp_edges


def write_result_to_file(filename, total_weight, adj_matrix, mst_edges):
    """Записывает результат в файл в требуемом формате."""
    with open(filename, 'w', encoding='utf-8') as f:
        # 1. Общий вес
        f.write(f"{int(total_weight) if total_weight == int(total_weight) else total_weight}\n")

        # 2. Матрица смежности MST
        for row in adj_matrix:
            f.write(", ".join(str(x) for x in row) + "\n")

        # 3. Список рёбер с номерами от 1, где левое число меньше правого
        edges_str = []
        for u, v, _ in mst_edges:
            u1, v1 = u + 1, v + 1
            if u1 > v1:
                u1, v1 = v1, u1
            edges_str.append(f"({u1}, {v1})")

        f.write(" ".join(edges_str) + "\n")


def print_matrix(matrix):
    """Красиво печатает матрицу с заменой inf на ∞"""
    n = len(matrix)
    print(f"Размер матрицы: {n}×{n}\n")
    print("Матрица весов (0 = отсутствие ребра, ∞ = inf):")
    for row in matrix:
        print("  ", end="")
        for val in row:
            if val == float('inf'):
                print("  ∞", end=" ")
            elif val == 0:
                print("  0", end=" ")
            else:
                print(f"{val:4.0f}" if val == int(val) else f"{val:4.1f}", end=" ")
        print()
    print()


# ========== Основная программа ==========

if __name__ == "__main__":
    # Запрашиваем имя входного файла
    filename = input("Введите имя файла с матрицей: ").strip()
    if not filename:
        filename = "matrix.txt"
        print(f"Используется файл по умолчанию: {filename}")

    # Читаем матрицу
    matrix = read_matrix_from_file(filename)

    if len(matrix) > 50:
        print(f"Предупреждение: размер матрицы {len(matrix)} превышает 50.")

    # Выводим матрицу
    print_matrix(matrix)

    # Строим MST для всех компонент, начиная с вершины 11 (индекс 10)
    start_vertex = 30  # Вершина 11 в 1-индексации
    print(f"Стартовая вершина: {start_vertex + 1} (индекс {start_vertex})")

    total_weight, mst_edges, mst_adj_matrix = prim_mst_all_components(matrix, start_vertex)

    # Результаты в консоль
    print("=" * 50)
    print("РЕЗУЛЬТАТ")
    print("=" * 50)
    print(f"Общий вес минимального остовного дерева: {total_weight}")
    print(f"Количество рёбер: {len(mst_edges)}")
    print("\nРёбра минимального остовного дерева:")

    edges_sorted = sorted(mst_edges, key=lambda x: (x[0], x[1]))
    for u, v, w in edges_sorted:
        print(f"  Вершины {u + 1:2} -- {v + 1:2}  |  вес: {w:7.2f}" if w != int(w)
              else f"  Вершины {u + 1:2} -- {v + 1:2}  |  вес: {w:7.0f}")

    # Проверка связности
    n = len(matrix)
    if len(mst_edges) < n - 1:
        print("\n ВНИМАНИЕ: Граф несвязный!")
        print(f"   Найдено {len(mst_edges)} рёбер из {n - 1} необходимых для полного дерева.")
        print(f"   Построен лес из нескольких компонент связности.")

    # Запись в файл
    write_result_to_file("result.txt", total_weight, mst_adj_matrix, mst_edges)
    verify_mst(matrix, total_weight, mst_edges)
    print("\nРезультат сохранён в файл 'result.txt'")