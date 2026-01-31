import trimesh
import numpy as np
from collections import defaultdict

# Get input file
Dir = input("Where is the .obj file you want to make into 4D?\n")
if not Dir.endswith('.obj'):
    Dir += '.obj'

Name = input("\nHow do you want to call the resulting file?\n")
if not Name.endswith('.txt'):
    Name += '.txt'

file = open(Name, 'w')
mesh = trimesh.load(Dir)

# Manually deduplicate vertices
unique_verts = []
vertex_map = {}
new_faces = []

for i, v in enumerate(mesh.vertices):
    v_tuple = tuple(v)
    if v_tuple not in vertex_map:
        vertex_map[v_tuple] = len(unique_verts)
        unique_verts.append(v)

# Remap faces to use deduplicated vertex indices
for face in mesh.faces:
    new_face = [vertex_map[tuple(mesh.vertices[idx])] for idx in face]
    new_faces.append(new_face)

unique_verts = np.array(unique_verts)
new_faces = np.array(new_faces)

print(f"Original mesh: {len(unique_verts)} vertices, {len(new_faces)} faces")

# Create 4D vertices: original set at w=-0.5, extruded set at w=0.5
n_verts = len(unique_verts)
vertices_4d_w_neg = np.hstack([unique_verts, -0.5 * np.ones((n_verts, 1))])
vertices_4d_w_pos = np.hstack([unique_verts, 0.5 * np.ones((n_verts, 1))])

# Combine both sets of vertices
all_vertices_4d = np.vstack([vertices_4d_w_neg, vertices_4d_w_pos])

# Build adjacency information: which vertices are connected via edges
edges = set()
for face in new_faces:
    # Each triangle has 3 edges
    edges.add(tuple(sorted([face[0], face[1]])))
    edges.add(tuple(sorted([face[1], face[2]])))
    edges.add(tuple(sorted([face[2], face[0]])))

print(f"Found {len(edges)} unique edges")

# Generate tetrahedra
tetrahedra = []

# 1. Add tetrahedra from the original faces (bottom caps)
for face in new_faces:
    v0, v1, v2 = face
    v0_top = v0 + n_verts
    v1_top = v1 + n_verts
    v2_top = v2 + n_verts
    
    # Decompose triangular prism into 3 tetrahedra
    # Using a consistent decomposition pattern
    tetrahedra.append([v0, v1, v2, v0_top])
    tetrahedra.append([v1, v2, v0_top, v1_top])
    tetrahedra.append([v2, v0_top, v1_top, v2_top])

print(f"Generated {len(tetrahedra)} tetrahedra from triangular prisms")

# 2. Connect corresponding vertices between layers
quad_tetrahedra = []
for edge in edges:
    v0, v1 = edge
    v0_top = v0 + n_verts
    v1_top = v1 + n_verts
    
    # Create tetrahedron from the quad edge
    quad_tetrahedra.append([v0, v1, v0_top, v1_top])

print(f"Generated {len(quad_tetrahedra)} tetrahedra from edge connections")

# Combine all tetrahedra
all_tetrahedra = tetrahedra + quad_tetrahedra

# Output 4D vertices
print("\n" + "="*60)
print("4D VERTICES")
print("="*60)
file.write("// 4D Vertices (x, y, z, w)\n")
file.write(f"// Total vertices: {len(all_vertices_4d)}\n\n")

vertices_list = []
for i, v in enumerate(all_vertices_4d):
    vert_str = ', '.join(f"{x}f" for x in v)
    vertices_list.append(f"{{{vert_str}}}")
    if i < 10 or i >= len(all_vertices_4d) - 10:  # Print first and last 10
        print(f"v{i}: {vert_str}")

# Add comma at the end of the vertex list
vertices_output = f"{{{', '.join(vertices_list)}}},"
file.write(vertices_output + "\n\n")

# Output tetrahedra with actual vertex coordinates
print("\n" + "="*60)
print("4D TETRAHEDRA (with vertex coordinates)")
print("="*60)
file.write("// 4D Tetrahedra (each tetrahedron contains 4 vertices with x, y, z, w coordinates)\n")
file.write(f"// Total tetrahedra: {len(all_tetrahedra)}\n\n")

# Flatten all vertex coordinates from all tetrahedra
all_tet_vertices = []
for tet_idx, tet in enumerate(all_tetrahedra):
    for vertex_idx in tet:
        v = all_vertices_4d[vertex_idx]
        vert_str = ', '.join(f"{x}f" for x in v)
        all_tet_vertices.append(f"{{{vert_str}}}")
    
    # Print first and last few tetrahedra for preview
    if tet_idx < 3:
        print(f"\nTetrahedron {tet_idx}:")
        for i, vertex_idx in enumerate(tet):
            v = all_vertices_4d[vertex_idx]
            vert_str = ', '.join(f"{x}f" for x in v)
            print(f"  Vertex {i}: {vert_str}")
    elif tet_idx == 3:
        print("\n... (more tetrahedra) ...")

# Output as one big list
tetrahedra_output = f"{{{', '.join(all_tet_vertices)}}}"
file.write(tetrahedra_output + "\n")

print("\n" + "="*60)
print("SUMMARY")
print("="*60)
print(f"Total 4D vertices: {len(all_vertices_4d)}")
print(f"Total tetrahedra: {len(all_tetrahedra)}")
print(f"Total vertex entries in tetrahedra list: {len(all_tet_vertices)}")
print(f"  - From triangular prisms: {len(tetrahedra)}")
print(f"  - From edge connections: {len(quad_tetrahedra)}")
print(f"\nOutput written to: {Name}")

file.close()