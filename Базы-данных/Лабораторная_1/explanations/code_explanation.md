# Объяснение кода лабораторной работы 1

## Структура кода

### Основные компоненты
1. Подключение к базе данных (psycopg2)
2. Выполнение запросов
3. Обработка ошибок

## Пример кода
```python
import psycopg2
conn = psycopg2.connect(host="localhost", database="students", user="postgres", password="pwd")
cur = conn.cursor()
cur.execute("SELECT * FROM student LIMIT 10;")
print(cur.fetchall())
cur.close()
conn.close()
```
